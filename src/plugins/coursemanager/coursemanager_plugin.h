#ifndef COURSEMANAGER_COURSEMANAGER_PLUGIN_H
#define COURSEMANAGER_COURSEMANAGER_PLUGIN_H

#include <kumir2-libs/extensionsystem/kplugin.h>
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/coursesinterface.h>
#include <QTextStream>

namespace Shared {
class ActorInterface;
}

namespace Widgets {
class DeclarativeSettingsPage;
}

class MainWindowTask;
class KumZadanie;
class courseModel;

namespace CourseManager
{

class Plugin :
	public ExtensionSystem::KPlugin,
	public Shared::CoursesInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
#ifndef KUMIR2_STATIC_PLUGINS // static build registers plugins by factory functions
	Q_PLUGIN_METADATA(IID "kumir2.CourseManager")
#endif
#endif
	Q_INTERFACES(Shared::CoursesInterface)

public:
	explicit Plugin();

	// from CoursesInterface
	virtual QWidget *mainWindow() const;
	virtual QList<QMenu *> menus() const;
	virtual QStringList getListOfCourses() const;
	virtual QAction *actionPerformCheck() const;
	virtual void activateCourseFromList(QString file);

	QString getText();
	void setParam(QString paramname, QString param) {};
	void setPreProgram(QVariant param);
	bool startNewTask(QStringList isps, KumZadanie *task);
	void startProgram(QVariant param, KumZadanie *task);

	void rebuildRescentMenu();

public slots:
	void setEnabled(bool value);
	void setTestingResult(ProgramRunStatus status, int value);
	void nextField();
	void prevField();
	void lockContrls();

private /*methods*/:
	// from PluginInterface
	virtual QString initialize(
		const QStringList &configurationArguments,
		const ExtensionSystem::CommandLine &runtimeArguments
	);
	virtual void saveSession() const;
	virtual void restoreSession();
	virtual void createPluginSpec();
	virtual void updateSettings(const QStringList &keys);
	virtual void changeCurrentDirectory(const QString &path);
	virtual void changeGlobalState(
		ExtensionSystem::GlobalState old,
		ExtensionSystem::GlobalState current
	);

	// from KPlugin
	virtual QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters() const;
	virtual QWidget *settingsEditorPage();
	virtual bool isSafeToQuit();
	virtual void start();

private /*methods*/:
	int loadCourseFromConsole(QString wbname, QString cbname);
	int checkTaskFromConsole(const int taskID);
	int loadWorkBook(QString wbfilename, QString cbname);
	Shared::ActorInterface *getActor(QString name);

	//Inserts into start program, techer part marker.
	void fixOldKumTeacherMark(QDataStream *ds);

	bool setTextFromFile(QString fname);
	void showError(QString err);
	void selectNext(KumZadanie *task);
	void checkNext(KumZadanie *task);

private /*fields*/:
	int isp_no, field_no;
	bool DISPLAY;

	QFile outFile;
	QFileInfo cur_courseFileInfo;
	QTextStream  resultStream;
	QList<QMenu *> MenuList;

	QWidget *mainWindow_;
	QAction *actionPerformCheck_;
	QAction *nextFld;
	QAction *prevFld;
	Widgets::DeclarativeSettingsPage *settingsEditorPage_;
	QMenu *courseMenu;
	QMenu *rescentMenu;
	MainWindowTask *MW;
	KumZadanie *cur_task;
	courseModel *course;
};

} // namespace CourseManager

#endif // COURSEMANAGER_COURSEMANAGER_PLUGIN_H
