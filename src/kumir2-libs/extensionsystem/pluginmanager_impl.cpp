#include "pluginmanager_impl.h"
#include <QPluginLoader>
#include <QDebug>
#include <algorithm>

#if defined(Q_OS_WIN32)
#if defined(__MINGW32__)
static const char *LIB_PREFIX = "lib";
#else
static const char *LIB_PREFIX = "";
#endif
static const char *LIB_SUFFIX = ".dll";
#elif defined(Q_OS_MAC)
static const char *LIB_PREFIX = "lib";
static const char *LIB_SUFFIX = ".dylib";
#else
static const char *LIB_PREFIX = "lib";
static const char *LIB_SUFFIX = ".so";
#endif

namespace ExtensionSystem
{

QMap<QByteArray, PluginManager::StaticPluginFactory> &staticPluginFactories()
{
	// Function-local static: safe to use from static initializers of other units
	static QMap<QByteArray, PluginManager::StaticPluginFactory> factories;
	return factories;
}


bool PluginManagerImpl::extractRuntimeParametersForPlugin(const KPlugin *plugin, CommandLine &parameters)
{
	parameters = CommandLine(plugin->acceptableCommandLineParameters());

	// Eat named parameters
	for (int i = 0; i < parameters.data_.size(); i++) {
		CommandLineParameter &param = parameters.data_[i];
		typedef std::list<QString>::iterator It;
		for (It it = namedProgramArguments.begin(); it != namedProgramArguments.end(); ++it) {
			QString &arg = *it;
			if (param.shortName_.isNull()) {

			} else if (arg.startsWith("--" + param.longName_) || arg.startsWith("-" + QString(param.shortName_))) {
				param.fillValue(arg);
				namedProgramArguments.erase(it);
				break;
			}
		}
	}

	// Eat unnamed parameters in case if plugin is startup module
	if (plugin == objects.last()) {
		CommandLineParameter *chain = nullptr;
		for (int i = 0; i < parameters.data_.size(); i++) {
			CommandLineParameter &param = parameters.data_[i];
			if (param.shortDescription_.length() > 0) {
				if (unnamedProgramArguments.size() > 0) {
					param.value_ = unnamedProgramArguments.front();
					unnamedProgramArguments.pop_front();
				}
				chain = param.valueRequired_ ? nullptr : &param;
			}
		}

		// Eat a list of unnamed parameters
		while (chain && unnamedProgramArguments.size() > 0) {
			CommandLineParameter param(
				chain->allowInGui_,
				chain->shortDescription_,
				chain->description_,
				chain->acceptType_,
				false);
			param.value_ = unnamedProgramArguments.front();
			unnamedProgramArguments.pop_front();
		}
	}

	bool success = true;
	for (int i = 0; i < parameters.data_.size(); i++) {
		CommandLineParameter &param = parameters.data_[i];
		if (param.shortDescription_.length() > 0) {
			success = success && (param.isValid() || plugin != objects.last());
		} else {
			success = success && param.isValid();
		}
	}

	return success;
}

void PluginManagerImpl::changeWorkingDirectory(const QString &path, bool saveChanges, bool workDirOnly)
{
	if (!workDirOnly) {
		settingsWorkspacePath = path;
		for (int i = 0; i < objects.size(); i++) {
			KPlugin *p = objects[i];
			SettingsPtr s = p->mySettings();
			if (s) {
				if (saveChanges) {
					p->saveSession();
				}
				s->changeWorkingDirectory(path);
			}
		}
	}
	QDir::root().mkpath(path);
	QDir::setCurrent(path);
	if (!workDirOnly) {
		QDir::current().mkdir(".settings");
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path + "/.settings");
		for (int i = 0; i < objects.size(); i++) {
			KPlugin *p = objects[i];
			p->changeCurrentDirectory(path);
			p->updateSettings(QStringList());
			p->restoreSession();
		}
	}
	mySettings->setValue(PluginManager::CurrentWorkspaceKey, path);
}

QString PluginManager::workspacePath() const
{
	return pImpl_->mySettings->value(PluginManager::CurrentWorkspaceKey).toString();
}


QString PluginManagerImpl::loadPlugin(PluginSpec spec, const QList<PluginSpec> &allSpecs)
{
	if (isPluginLoaded(spec.name)) {
		return "";
	}
	QList<QByteArray> blacklist;
#ifdef Q_OS_UNIX
	const char *kumirBlacklist = getenv("KUMIR_BLACKLIST");
	blacklist = QByteArray(kumirBlacklist).split(':');
#endif
	if (blacklist.contains(spec.name)) {
		qDebug() << "Plugin " << spec.name << " not loaded because of blacklisted";
		return "";
	}

	Q_FOREACH (const QByteArray &depName, spec.dependencies) {
		bool specFound = false;
		PluginSpec specToLoad;
		Q_FOREACH (const PluginSpec &anotherSpec, allSpecs) {
			if (anotherSpec.name == depName) {
				specToLoad = anotherSpec;
				specFound = true;
				break;
			}
		}
		if (!specFound) {
			return "Can't find spec for dependency: " + depName;
		}
		loadPlugin(specToLoad, allSpecs);
	}

#ifdef KUMIR2_STATIC_PLUGINS
	// Plugins are linked into the executable: create by registered factory
	const QString nonStandardPathToUse;
	PluginManager::StaticPluginFactory factory = staticPluginFactories().value(spec.name, 0);
	if (!factory) {
		return QString("Can't load module %1: not linked into this executable")
			.arg(QString::fromLatin1(spec.name));
	}
	KPlugin *plugin = factory();
	if (!plugin) {
		return QString("Plugin %1 is not valid (does not implement interface KPlugin)");
	}
#else
	QStringList librarySearchPaths = QStringList() << path;
#ifdef Q_OS_UNIX
	const char *extraPluginSearchPath = ::getenv("KUMIR2_PLUGIN_PATH");
	if (extraPluginSearchPath) {
		librarySearchPaths.append(QString::fromUtf8(extraPluginSearchPath).split(":"));
	}
#endif
	Q_FOREACH (const QString &prefix, additionalPluginPrefixes) {
		librarySearchPaths.append(prefix + "/lib64/kumir2/plugins");
	}
	Q_FOREACH (const QString &prefix, additionalPluginPrefixes) {
		librarySearchPaths.append(prefix + "/lib/kumir2/plugins");
	}

	QString libraryPath;
	QString nonStandardPathToUse;
	for (int i = 0; i < librarySearchPaths.size(); ++i) {
		const QString &dirName = librarySearchPaths.at(i);
		const QString candidate = dirName +
			QString("/") +
			QString(LIB_PREFIX) +
			spec.name +
			QString(LIB_SUFFIX);
		const QFileInfo fi(candidate);
		if (fi.exists()) {
			libraryPath = candidate;
			if (i > 0) {
				nonStandardPathToUse = dirName;
			}
			break;
		}
	}

	spec.libraryFileName = libraryPath.toUtf8();

	QPluginLoader loader(spec.libraryFileName);
	if (!loader.load()) {
		return QString("Can't load module %1: %2")
			.arg(QString::fromLatin1(spec.name))
			.arg(loader.errorString());
	}
	KPlugin *plugin = qobject_cast<KPlugin *>(loader.instance());
	if (!plugin) {
		return QString("Plugin %1 is not valid (does not implement interface KPlugin)");
		loader.unload();
	}
#endif
	plugin->createPluginSpec();
	plugin->_pluginSpec.arguments = spec.arguments;
	plugin->_pluginSpec.main = spec.main;
	plugin->_pluginSpec.nonStandardPluginDir = nonStandardPathToUse;
	plugin->_state = KPlugin::Loaded;
	plugin->_settings = SettingsPtr(new Settings(QString::fromLatin1(spec.name)));
	objects.append(plugin);
	return "";
}

QString PluginManagerImpl::initializePlugin(KPlugin *entryPoint)
{
	QString error;
	KPlugin::State state = entryPoint->state();
	if (KPlugin::Initialized == state || KPlugin::Disabled == state) {
		return "";
	}
	const PluginSpec &spec = entryPoint->pluginSpec();
	QList<KPlugin *> deps;
	Q_FOREACH (const QByteArray &depName, spec.dependencies) {
		Q_FOREACH (KPlugin *anotherPlugin, objects) {
			const PluginSpec &anotherSpec = anotherPlugin->pluginSpec();
			if (anotherSpec.name == depName || anotherSpec.provides.contains(depName)) {
				deps.append(anotherPlugin);
			}
		}
	}

	Q_FOREACH (KPlugin *dep, deps) {
		error = initializePlugin(dep);
		if (error.length() > 0) {
			return error;
		}
	}

	CommandLine runtimeParameters;
	if (!extractRuntimeParametersForPlugin(entryPoint, runtimeParameters)) {
		error = PluginManager::tr("The following command line parameters required, but not set:\n");
		for (int i = 0; i < runtimeParameters.data_.size(); i++) {
			const CommandLineParameter &param = runtimeParameters.data_[i];
			if (!param.isValid()) {
				error += "  " + param.toHelpLine() + "\n";
			}
		}
		error += PluginManager::tr("Run with --help for more details.\n");
		return error;
	}

	qDebug() << "Initializing plugin" << entryPoint->pluginName();
	error = entryPoint->initialize(spec.arguments, runtimeParameters);
	if (error.length() == 0) {
		entryPoint->_state = KPlugin::Initialized;
	}
	return error;
}


bool PluginManagerImpl::isPluginLoaded(const QByteArray &name) const
{
	Q_FOREACH (const KPlugin *plugin, objects) {
		Q_ASSERT(plugin);
		if (plugin->pluginSpec().name == name) {
			return true;
		}
	}
	return false;
}

QString PluginManagerImpl::parsePluginsRequest(const QByteArray &templ, QList<PluginSpec> &plugins)
{
	if (templ.trimmed().isEmpty()) {
		return "Plugins template is empty";
	}
	bool inBr = false;
	PluginSpec cur;
	QString argument;
	cur.main = false;
	for (int i = 0 ; i < templ.size(); i++) {
		if (templ[i] == '!' && cur.name.isEmpty() && !inBr) {
			cur.main = true;
		} else if (templ[i] == '(' && !inBr) {
			inBr = true;
		} else if (templ[i] == ')' && inBr) {
			inBr = false;
			cur.arguments << argument;
			argument = "";
		} else if (templ[i] == ',' && inBr) {
			cur.arguments << argument;
			argument = "";
		} else if (templ[i] == ',' && !inBr) {
			plugins << cur;
			cur.main = false;
			cur.arguments.clear();
			cur.name = "";
		} else if (inBr) {
			argument += templ[i];
		} else {
			cur.name += templ[i];
		}
	}
	plugins << cur;
	int starts = 0;
	// Qt bug !!! Use STL instead and then convert back to QList
#if QT_VERSION >= 0x060000
	std::list<PluginSpec> stdPlugins(plugins.begin(), plugins.end());
#else
	std::list<PluginSpec> stdPlugins = plugins.toStdList();
#endif
	std::list<PluginSpec>::iterator it = stdPlugins.begin();
	while (it != stdPlugins.end()) {
		PluginSpec p = (*it);
		if (p.name.contains("*") || p.name.contains("?")) {
			if (p.main) {
				return "Entry point defined for masked by * name in plugins template";
			}
#ifdef KUMIR2_STATIC_PLUGINS
			// Same matching and order as QDir::entryList below gives for plugin
			// libraries: case insensitive wildcard, sorted by lower-cased file name
			QStringList entries;
			const QString nameFilter = LIB_PREFIX + p.name + LIB_SUFFIX;
			Q_FOREACH (const QByteArray &staticName, staticPluginFactories().keys()) {
				const QString fileName = LIB_PREFIX + QString::fromLatin1(staticName) + LIB_SUFFIX;
				if (QDir::match(nameFilter, fileName)) {
					entries.append(fileName);
				}
			}
			std::sort(entries.begin(), entries.end(), [](const QString &a, const QString &b) {
				return a.toLower() < b.toLower();
			});
#else
			QDir dir(path);
			QStringList entries = dir.entryList(QStringList() <<
					LIB_PREFIX + p.name + LIB_SUFFIX,
					QDir::Files);
#endif
			it = stdPlugins.erase(it);
			foreach (const QString &e, entries) {
				PluginSpec pp;
				QByteArray pluginName = e
					.mid(QByteArray(LIB_PREFIX).length(),
						e.length() - QByteArray(LIB_PREFIX).length() - QByteArray(LIB_SUFFIX).length())
					.toLocal8Bit();
				pp.name = pluginName;
				pp.arguments = p.arguments;
				pp.main = false;
				stdPlugins.insert(it, pp);
			}
		} else {
			++it;
		}
	}
#if QT_VERSION >= 0x060000
	plugins = QList<PluginSpec>(stdPlugins.begin(), stdPlugins.end());
#else
	plugins = QList<PluginSpec>::fromStdList(stdPlugins);
#endif
	for (int i = 0; i < plugins.size(); i++) {
		if (plugins[i].main) {
			starts ++;
			mainPluginName = plugins[i].name;
		}
	}

	if (starts > 1) {
		return "More than one entry point defined in plugins template";
	}
	if (starts == 0) {
		return "Entry point is not defined in plugins template";
	}
	return "";
}

} // namespace ExtensionSystem
