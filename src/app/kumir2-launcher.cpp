#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QTranslator>
#include <QDir>
#include <QIcon>
#include <QPainter>
#include <QSplashScreen>
#include <QApplication>
#include <QTextCodec>
#ifdef Q_OS_WASM
#include <emscripten.h>

// Qt 6.10 installs clipboard handlers on its canvas in some browsers,
// while a custom QWidget editor receives DOM focus in the sibling helper.
// Forward only that helper's paste events to Qt's existing clipboard path.
EM_JS(void, installWasmPasteBridge, (), {
    document.addEventListener("paste", function(event) {
        const target = event.composedPath()[0];
        if (!target || !target.classList ||
            !target.classList.contains("qt-window-focus-helper"))
            return;
        const windowElement = target.closest(".qt-window");
        const canvas = windowElement && windowElement.querySelector("canvas.qt-window-canvas");
        if (!canvas)
            return;
        // Native DataTransfer contents are only readable during the browser's
        // paste dispatch. Asyncify can defer Qt's callback until afterward.
        const contents = new DataTransfer();
        if (!event.clipboardData)
            return;
        for (const format of event.clipboardData.types) {
            if (format !== "Files")
                contents.setData(format, event.clipboardData.getData(format));
        }
        for (const file of event.clipboardData.files)
            contents.items.add(file);
        const forwarded = new Event("paste", {
            bubbles: true, composed: true, cancelable: true
        });
        // Firefox ignores ClipboardEventInit.clipboardData.
        Object.defineProperty(forwarded, "clipboardData", { value: contents });
        canvas.dispatchEvent(forwarded);
        // Qt may consume the forwarded event asynchronously. Avoid delivering
        // the original a second time or inserting into the hidden DOM helper.
        event.preventDefault();
        event.stopImmediatePropagation();
    }, true);
});
#endif

#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2-libs/extensionsystem/logger.h>


static bool guiMode = false, pipeMode = false;
static bool versionMode = false, helpMode = false;

#ifdef CONFIGURATION_TEMPLATE
static QByteArray currentTemplate = CONFIGURATION_TEMPLATE;
#else
#error No default configuration passed to GCC
#endif

// template should be the same as in the launchers
static struct AsIf {
	const char *role;
	const char *tmpl;
} asifs[] = {
	{"teacher", "CourseManager,Editor(teacher),Actor*,KumirAnalizer(teacher,preload=Files),KumirCodeGenerator,KumirCodeRun,!CoreGUI"},
	{"ide",     "CourseManager,Editor,Actor*,KumirAnalizer(preload=Files),*CodeGenerator,KumirCodeRun,!CoreGUI"},
	{"classic", "CourseManager,Editor,Actor*,KumirAnalizer(preload=Files),*CodeGenerator,KumirCodeRun,!CoreGUI(notabs,nostartpage,nosessions)"},
	{"checkcourse", "ActorRobot,!CourseManager,KumirCodeGenerator,KumirCodeRun(console),KumirAnalizer(teacher,preload=Files,preload=Strings)"},
	{"bc", "Actor*(tablesOnly),!KumirCompilerTool,KumirCodeGenerator,KumirAnalizer(teacher,preload=Files,preload=Strings)"},
	{"xrun", "!KumirCodeRun(console),ActorRobot"},
};
static int asifs_size = sizeof(asifs) / sizeof(asifs[0]);


static void parse_options(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		QByteArray arg = argv[i];
		if (arg == "--help" || arg == "-h" || arg == "/?") {
			helpMode = true;
		} else if (arg == "--version") {
			versionMode = true;
		} else if (arg == "--pipe" || arg == "-p") {
			pipeMode = true;
		} else if (arg == "--nopipe") {
			pipeMode = false;
		} else if (arg == "--gui") {
			guiMode = true;
		} else if (arg == "--nogui") {
			guiMode = false;
		} else if (arg.startsWith("--template=")) {
			currentTemplate = arg.mid(11);
		} else if (arg.startsWith("--asif=")) {
			QByteArray asif = arg.mid(7);
			bool found = false;
			for (int i = 0; i < asifs_size; i++) {
				if (asifs[i].role == asif) {
					currentTemplate = asifs[i].tmpl;
					found = true;
					break;
				}
			}
			if (!found) {
				fprintf(stderr, "Unknown role '%s'\n", asif.data());
			}
		} else if (!arg.startsWith("-")) {
			break;
		}
	}
}

static QString resolvePath(const char *what)
{
	static QString ExecDir = QString::fromLatin1(KUMIR2_EXEC_DIR);
	static QString MyDir = QCoreApplication::applicationDirPath();
	static int RootDistLevelUp = ExecDir.isEmpty()
		? 0
		: ExecDir.count("/") + 1;
	QString result = MyDir;
	for (int i = 0; i < RootDistLevelUp; ++i) {
		result += "/../";
	}
	result += QString::fromLatin1(what);
	result = QDir::cleanPath(result);
	return result;
}

#if QT_VERSION < 0x050000
static void LoggerMessageOutput(QtMsgType type, const char *msga)
{
	QString msg(QString::fromLocal8Bit(msga));
#else
static void LoggerMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
#endif
	ExtensionSystem::Logger *logger = ExtensionSystem::Logger::instance();
	switch (type) {
	case QtDebugMsg:
		logger->debug(msg);
		break;
	case QtWarningMsg:
		logger->warning(msg);
		break;
	case QtCriticalMsg:
		logger->critical(msg);
		break;
	case QtFatalMsg:
		logger->fatal(msg);
		abort();
	default:
		break;
	}
}

#if QT_VERSION < 0x050000
static void ConsoleMessageOutput(QtMsgType type, const char *msga)
{
	QString msg(QString::fromLocal8Bit(msga));
#else
static void ConsoleMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
#endif
	switch (type) {
	case QtDebugMsg:
//        fprintf(stderr, "Debug: %s\n", msg);
		break;
	case QtWarningMsg:
//        fprintf(stderr, "Warning: %s\n", msg);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "Critical: %s\n", msg.toLocal8Bit().data());
		break;
	case QtFatalMsg:
		fprintf(stderr, "Fatal: %s\n", msg.toLocal8Bit().data());
		abort();
	default:
		break;
	}
}

static void showErrorMessage(const QString &text)
{
	if (guiMode) {
		QMessageBox::critical(0, "Kumir 2 Launcher", text);
	} else {
		fprintf(stderr, "%s\n", qPrintable(text));
	}
}

static QString getLanguage()
{
	return "ru"; // TODO implement sometime in future...
}

static
QList<QDir> translationsDirs()
{
	QList<QDir> result;

	// Translations dir from base distribution
	QString sharePath = resolvePath(KUMIR2_RESOURCES_DIR);
	QDir baseTranslationsDir(sharePath + "/translations");
	if (baseTranslationsDir.exists()) {
		result.append(baseTranslationsDir);
	}

#ifdef Q_OS_LINUX
	// Search additional paths
	QString homePath = QString::fromLocal8Bit(::getenv("HOME"));
	QStringList extraPaths = QStringList()
		<< "/usr/share/kumir2/translations"
		<< "/usr/local/share/kumir2/translations"
		<< "/opt/kumir2/share/translations"
		<< "/opt/kumir/share/translations"
		<< homePath + "/.local/share/kumir2/translations"
		<< QDir::currentPath() + "/translations"
		;

	Q_FOREACH (const QString &path, extraPaths) {
		QDir candidate(path);
		if (candidate.exists()) {
			result.append(candidate);
		}
	}
#endif

	return result;
}

class Application : QObject
{
public:
	explicit Application(int &argc, char **argv, bool gui)
		: QObject()
		, _qApp(0)
		, _timerId(-1)
		, _splashScreen(nullptr)
		, _started(false)
		, _gui(gui)
	{
		if (_gui) {
			_qApp = new QApplication(argc, argv);
		} else {
			_qApp = new QCoreApplication(argc, argv);
		}
		_qApp->installEventFilter(this);
	}

	void setSplashScreen(QSplashScreen *s)
	{
		_splashScreen = s;
	}

	void initialize()
	{
		QStringList arguments = _qApp->arguments();
		qDebug() << "Arguments: " << arguments;

		const QList<QDir> tsDirs = translationsDirs();
		Q_FOREACH (const QDir &translationsDir, tsDirs) {
			QStringList ts_files = translationsDir.entryList(QStringList() << "*_" + getLanguage() + ".qm");
			foreach (QString tsname, ts_files) {
				tsname = tsname.mid(0, tsname.length() - 3);
				QTranslator *tr = new QTranslator(_qApp);
				if (tr->load(tsname, translationsDir.absolutePath())) {
					_qApp->installTranslator(tr);
				}
			}
		}

		QString sharePath = resolvePath(KUMIR2_RESOURCES_DIR);
		_qApp->setProperty("sharePath", sharePath);

		QSettings::setDefaultFormat(QSettings::IniFormat);
		_qApp->addLibraryPath(resolvePath(KUMIR2_LIBS_DIR));
		_qApp->addLibraryPath(resolvePath(KUMIR2_PLUGINS_DIR));
		ExtensionSystem::PluginManager *manager = ExtensionSystem::PluginManager::instance();
		manager->setPluginPath(resolvePath(KUMIR2_PLUGINS_DIR));
		manager->setSharePath(sharePath);
		QString error;
		qDebug() << "Initialized plugin manager";

		QByteArray templ = currentTemplate;
		for (int i = 1; i < arguments.size(); i++) {
			QByteArray arg = arguments[i].toLatin1();
			if (arg.startsWith("[") && arg.endsWith("]")) {
				templ = arg.mid(1, arg.length() - 2);
			}
		}
		qDebug() << "Loading plugins by template: " << templ;
		error = manager->loadPluginsByTemplate(templ);
		if (!_gui && manager->isGuiRequired()) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			showErrorMessage("Requires X11 session to run this configuration");
			_qApp->exit(1);
			return;
		}

		if (!error.isEmpty()) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			showErrorMessage(error);
			_qApp->exit(1);
			return;
		}

		qDebug() << "Done loading all plugins by template";

		if (helpMode) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			QString msg = manager->commandLineHelp();
#ifdef Q_OS_WIN32
			QTextCodec *codec = QTextCodec::codecForName("CP866");
			fprintf(stderr, "%s", codec->fromUnicode(msg).constData());
#else
			fprintf(stderr, "%s", msg.toLocal8Bit().data());
#endif
			_qApp->exit(0);
			return;
		}

		if (versionMode) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			fprintf(stderr, "%s\n", qPrintable(_qApp->applicationVersion()));
			_qApp->exit(0);
			return;
		}

		qDebug() << "Begin plugins initialization";
		error = manager->initializePlugins();
		if (!error.isEmpty()) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			showErrorMessage(error);
			_qApp->exit(_qApp->property("returnCode").isValid()
				? _qApp->property("returnCode").toInt() : 1);
			return;
		}

		// GUI requirement may be changed as result of plugins initialization,
		// so check it again
		qDebug() << "Plugins initialization done";
		if (!_gui && manager->isGuiRequired()) {
			if (_splashScreen) {
				_splashScreen->finish(0);
			}
			showErrorMessage("Requires X11 session to run this configuration");
			_qApp->exit(_qApp->property("returnCode").isValid()
				? _qApp->property("returnCode").toInt() : 1);
			return;
		}

		if (_splashScreen) {
			_splashScreen->finish(0);
			_splashScreen = 0;
		}

		qDebug() << "Starting entry point plugin";
		error = manager->start();
		if (!error.isEmpty()) {
			showErrorMessage(error);
			_qApp->exit(_qApp->property("returnCode").isValid()
				? _qApp->property("returnCode").toInt() : 1);
		}

		if (!manager->isGuiRequired()) {
			_qApp->quit();
		}
	}

	bool eventFilter(QObject *obj, QEvent *event)
	{
		if (event->type() == QEvent::Timer && obj == _qApp && !_started) {
			_started = true;
			_qApp->killTimer(_timerId);
			qDebug() << "Begin initialization";
			initialize();
			qDebug() << "Initialization done";
			return true;
		} else {
			return QObject::eventFilter(obj, event);
		}
	}

	int main()
	{
		int dt = _splashScreen ? 250 : 50;
		_timerId = _qApp->startTimer(dt);
		int ret = _qApp->exec();
		if (ret == 0) {
			return _qApp->property("returnCode").isValid()
				? _qApp->property("returnCode").toInt() : 0;
		} else {
			return ret;
		}
	}

private:
	QCoreApplication *_qApp;
	int _timerId;
	QSplashScreen *_splashScreen;
	bool _started, _gui;
};


static bool
setup_custom_vendor_information()
{
	QString appName;
	QString appVendor;
	QString appVersion;
	QString appLicenseFileName;
	QString appAboutFileName;
	bool result = false;
#ifdef APP_NAME
	appName = QString::fromLatin1(APP_NAME);
	result = true;
#endif
#ifdef APP_NAME_ru
	appName = QString::fromUtf8(APP_NAME_ru);
	result = true;
#endif
#ifdef APP_VENDOR
	appVendor = QString::fromLatin1(APP_VENDOR);
#endif
#ifdef APP_VENDOR_RU
	appVendor = QString::fromUtf8(APP_VENDOR_ru);
#endif
#ifdef APP_VERSION
	appVersion = QString::fromLatin1(APP_VERSION);
#endif
#ifdef APP_LICENSE
	appLicenseFileName = ":/kumir2-launcher/" + QString::fromLatin1(APP_LICENSE);
#endif
#ifdef APP_LICENSE_ru
	appLicenseFileName = ":/kumir2-launcher/" + QString::fromLatin1(APP_LICENSE_ru);
#endif
#ifdef APP_ABOUT
	appAboutFileName = ":/kumir2-launcher/" + QString::fromLatin1(APP_ABOUT);
#endif
#ifdef APP_ABOUT_ru
	appAboutFileName = ":/kumir2-launcher/" + QString::fromLatin1(APP_ABOUT_ru);
#endif
	if (appName.length() > 0) {
		qApp->setProperty("customAppName", appName);
	}
	if (appVendor.length() > 0) {
		qApp->setProperty("customAppVendor", appVendor);
	}
	if (appVersion.length() > 0) {
		qApp->setProperty("customAppVersion", appVersion);
	}
	if (appLicenseFileName.length() > 0) {
		qApp->setProperty("customAppLicense", appLicenseFileName);
	}
	if (appAboutFileName.length() > 0) {
		qApp->setProperty("customAppAbout", appAboutFileName);
	}
	return result;
}


int main(int argc, char **argv)
{
#ifdef Q_OS_WASM
	// Emscripten defaults argv[0] to ./this.program. Kumir derives its
	// settings scope and documentation index from the executable name.
	static char launcherName[] = KUMIR2_WASM_LAUNCHER_NAME;
	if (argc > 0) {
		argv[0] = launcherName;
	}
#endif
	guiMode = false;
	pipeMode = false;
	versionMode = false;
	helpMode = false;

#if defined(WINDOW_ICON) || defined(SPLASHSCREEN)
	guiMode = true;
#endif

#ifdef Q_OS_LINUX
	guiMode = guiMode && getenv("DISPLAY") != 0;
#endif


	parse_options(argc, argv);

#if QT_VERSION < 0x050000
	qInstallMsgHandler((!pipeMode) ? LoggerMessageOutput : ConsoleMessageOutput);
#else
	qInstallMessageHandler((!pipeMode) ? LoggerMessageOutput : ConsoleMessageOutput);
#endif

	Application *app = new Application(argc, argv, guiMode);
#ifdef Q_OS_WASM
	installWasmPasteBridge();
#endif

#ifdef WINDOW_ICON
	if (guiMode) {
		QApplication *guiApp = qobject_cast<QApplication *>(qApp);
		QString imgPath = ":/kumir2-launcher/" + QString::fromLatin1(WINDOW_ICON);
		QIcon icon(imgPath);
		guiApp->setWindowIcon(icon);
	}
#endif

	QLocale russian = QLocale("ru_RU");
	QLocale::setDefault(russian);

	qApp->addLibraryPath(resolvePath(KUMIR2_LIBS_DIR));
	qApp->addLibraryPath(resolvePath(KUMIR2_PLUGINS_DIR));

	QString gitHash = QString::fromLatin1(GIT_HASH);
	QString gitTag = QString::fromLatin1(GIT_TAG);
	QString gitBranch = QString::fromLatin1(GIT_BRANCH);
#if QT_VERSION >= 0x060000
	QDateTime gitTimeStamp = QDateTime::fromSecsSinceEpoch(QString::fromLatin1(GIT_TIMESTAMP).toUInt());
#else
	QDateTime gitTimeStamp = QDateTime::fromTime_t(QString::fromLatin1(GIT_TIMESTAMP).toUInt());
#endif

	qApp->setApplicationVersion(gitTag.length() > 0 && gitTag != "unknown"
		? gitTag : gitBranch + "/" + gitHash);
	qApp->setProperty("gitTimeStamp", gitTimeStamp);

	bool customInfo = setup_custom_vendor_information();
	Q_UNUSED(customInfo);

#ifdef SPLASHSCREEN
	QSplashScreen *splashScreen = 0;
	if (guiMode && !helpMode && !versionMode) {
		QString imgPath = ":/kumir2-launcher/" + QString::fromLatin1(SPLASHSCREEN);
		splashScreen = new QSplashScreen();
		QImage img(imgPath);
		QPainter p(&img);
		p.setPen(QColor(Qt::black));
		p.setBrush(QColor(Qt::black));
		QFont f = p.font();

		f.setPixelSize(12);
		p.setFont(f);

		QString v;
		if (customInfo) {
			v = qApp->property("customAppVersion").toString() + " ";
			v += "[based on Kumir ";
		}
		v = qApp->applicationVersion();
		if (qApp->property("gitHash").isValid()) {
			v += " (GIT " + qApp->property("gitHash").toString() + ")";
		}
		if (customInfo) {
			v += "]";
		}
#if QT_VERSION >= 0x060000
		int tw = QFontMetrics(f).horizontalAdvance(v);
#else
		int tw = QFontMetrics(f).width(v);
#endif
		int th = QFontMetrics(f).height() * 5;
		int x = img.width() - tw - 8;
		int y = 8;
		p.drawText(x, y, tw, th, 0, v);
		p.end();
		QPixmap px = QPixmap::fromImage(img);
		splashScreen->setPixmap(px);
		splashScreen->show();
		app->setSplashScreen(splashScreen);
		qApp->processEvents();
	}
#endif

	int ret = app->main();
	ExtensionSystem::PluginManager::destroy();
	delete app;
	return ret;
}

