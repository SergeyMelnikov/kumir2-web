/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#include "vodoleymodule.h"
#include "vodoley.h"
#include "pult.h"

#include <kumir2-libs/widgets/pultlogger.h>
#include <kumir2-libs/extensionsystem/kplugin.h>

#include <QMenu>
#include <QLayout>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

namespace ActorVodoley
{

VodoleyModule *VodoleyModule::self = 0;

VodoleyModule::VodoleyModule(ExtensionSystem::KPlugin *parent) :
	VodoleyModuleBase(parent)
{
	self = this;
	MainWindow = 0;
}

void VodoleyModule::createGui()
{
	MainWindow = new Vodoley();
	// Module constructor, called once on plugin load
	// TODO implement me
	QList<QAction *> actions;
	actions.append(m_actionVodoleyNewEnvironment);
	actions.append(m_actionVodoleyLoadEnvironment);
	actions.append(m_actionVodoleySaveEnvironment);
	MainWindow->createActions(actions);
	connect(m_actionVodoleyRevertEnvironment, SIGNAL(triggered()), MainWindow, SLOT(reset()));

	recentMenu = new QMenu();
	createRecentMenu();
	m_actionVodoleyLoadRescent->setMenu(recentMenu);
	connect(MainWindow, SIGNAL(FileLoaded(QString)), this, SLOT(updateLastFiles(QString)));
}

QString VodoleyModule::initialize(
	const QStringList &configurationParameters,
	const ExtensionSystem::CommandLine &
) {
	if (!configurationParameters.contains("tablesOnly")) {
		createGui();
		createRecentMenu();

		redrawTimer = new QTimer(this);
		connect(redrawTimer, SIGNAL(timeout()), MainWindow, SLOT(redraw()));
	}

	return "";
}

QList<ExtensionSystem::CommandLineParameter> VodoleyModule::acceptableCommandLineParameters()
{
	return QList<ExtensionSystem::CommandLineParameter>();
}

void VodoleyModule::changeGlobalState(
	ExtensionSystem::GlobalState old,
	ExtensionSystem::GlobalState current
) {
	// Called when changed kumir state. The states are defined as enum ExtensionSystem::GlobalState:
	/*
	namespace ExtensionSystem {
	    enum GlobalState {
	        GS_Unlocked, // Edit mode
	        GS_Observe, // Observe mode
	        GS_Running, // Running mode
	        GS_Input,  // User input required
	        GS_Pause  // Running paused
	    };
	}
	*/
	// TODO implement me
	//using namespace ExtensionSystem;  // not to write "ExtensionSystem::" each time in this method scope
	Q_UNUSED(old);  // Remove this line on implementation
	Q_UNUSED(current);  // Remove this line on implementation
	MainWindow->redraw();
	if (current == ExtensionSystem::GlobalState::GS_Running) {
		redrawTimer->start(100);
	} else {
		redrawTimer->stop();
	}
}

void VodoleyModule::loadActorData(QIODevice *source)
{
	// Set actor specific data (like environment)
	// The source should be ready-to-read QIODevice like QBuffer or QFile

	MainWindow->loadIoDevice(source);
	MainWindow->pult->pltLogger()->ClearLog();
	MainWindow->reset();
}

void VodoleyModule::tryToSleep()
{
#if 0
	if ((rand() % 100 < 5)) {
		msleep(2);
	}
#endif
}

QWidget *VodoleyModule::mainWidget() const
{
	return  MainWindow;
}

class AAA : public QWidget
{
public:
	explicit AAA(QWidget *pult_): QWidget(), pult(pult_)
	{
		setLayout(new QVBoxLayout);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->addWidget(pult);
	}

	QSize minimumSizeHint() const
	{
		return pult ? pult->minimumSizeHint() : QSize();
	}

private:
	QWidget *pult;
};

QWidget *VodoleyModule::pultWidget() const
{
	// Returns module control view widget, or nullptr if there is no control view
	// NOTE: the method is const and might be called at any time,
	//       so DO NOT create widget here, just return!
	// TODO implement me

	if (MainWindow == 0)
		return 0;

	VodoleyPult *pult = MainWindow->pult;

	if (pult == 0)
		return 0;

	qDebug() << "Pult" << pult->size();
	qDebug() << "Pult visible " << pult->isVisible();


	static QWidget *dummy = new AAA(pult);
	return dummy;
//    return MainWindow->pult;
	// return nullptr;
}

void VodoleyModule::reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList &keys)
{
	// Updates setting on module load, workspace change or appliyng settings dialog.
	// If @param keys is empty -- should reload all settings, otherwise load only setting specified by @param keys
	// TODO implement me
	my_settings = settings;
	createRecentMenu();
	Q_UNUSED(settings);  // Remove this line on implementation
	Q_UNUSED(keys);  // Remove this line on implementation
}

void VodoleyModule::reset()
{
	// Resets module to initial state before program execution
	// TODO implement me
	MainWindow->reset();
}

void VodoleyModule::setAnimationEnabled(bool enabled)
{
	// Sets GUI animation flag on run
	// NOTE this method just setups a flag and might be called anytime, even module not needed
	// TODO implement me
	Q_UNUSED(enabled);  // Remove this line on implementation
}

void VodoleyModule::runFillA()
{
	/* алг наполни A */
	// TODO implement me

	mutex.lock();
	MainWindow->FillA();
	mutex.unlock();
	tryToSleep();

}

void VodoleyModule::runFillB()
{
	/* алг наполни B */
	// TODO implement me
	mutex.lock();
	MainWindow->FillB();
	mutex.unlock();
	tryToSleep();
}

/* public slot */ void VodoleyModule::runFillC()
{
	/* алг наполни C */
	// TODO implement me
	mutex.lock();
	MainWindow->FillC();
	mutex.unlock();
	tryToSleep();

}

void VodoleyModule::runEmptyA()
{
	/* алг вылей A */
	// TODO implement me

	mutex.lock();
	MainWindow->MoveFromTo(0, 3); //Move water from A to ....
	mutex.unlock();
	tryToSleep();
}

void VodoleyModule::runEmptyB()
{
	/* алг вылей B */
	// TODO implement me
	mutex.lock();
	MainWindow->MoveFromTo(1, 3);
	//msleep(1);
	mutex.unlock();
	tryToSleep();
}

void VodoleyModule::runEmptyC()
{
	/* алг вылей C */
	// TODO implement me
	mutex.lock();
	MainWindow->MoveFromTo(2, 3);
	tryToSleep();

	mutex.unlock();
}

void VodoleyModule::runFromAToB()
{
	/* алг перелей из A в B */
	// TODO implement me
	//   mutex.lock();
	MainWindow->MoveFromTo(0, 1);
	tryToSleep();
	//  mutex.unlock();
}

void VodoleyModule::runFromAToC()
{
	/* алг перелей из A в C */

//    mutex.lock();
	MainWindow->MoveFromTo(0, 2);
	tryToSleep();
	//mutex.unlock();
}

void VodoleyModule::runFromBToA()
{
	/* алг перелей из B в A */

	//   mutex.lock();
	MainWindow->MoveFromTo(1, 0);
	tryToSleep();
	//   mutex.unlock();
}

void VodoleyModule::runFromBToC()
{
	/* алг перелей из B в C */
	// TODO implement me
	//  mutex.lock();
	MainWindow->MoveFromTo(1, 2);
	tryToSleep();
	//   mutex.unlock();
}

void VodoleyModule::runFromCToB()
{
	/* алг перелей из C в B */
	// TODO implement me
	//   mutex.lock();
	MainWindow->MoveFromTo(2, 1);
	tryToSleep();
	//   mutex.unlock();
}

void VodoleyModule::runFromCToA()
{
	/* алг перелей из C в A */
	// TODO implement me
	//   mutex.lock();
	MainWindow->MoveFromTo(2, 0);
	tryToSleep();
//    mutex.unlock();
}

bool VodoleyModule::runTaskComplited()
{
	/* алг лог @@задание выполненно */
	// TODO implement me
	return MainWindow->ready();
}

int VodoleyModule::runSizeA()
{
	return MainWindow->Asize();
}

int VodoleyModule::runSizeB()
{
	return MainWindow->Bsize();
}

int VodoleyModule::runSizeC()
{
	return MainWindow->Csize();
}

int VodoleyModule::runInA()
{
	return MainWindow->CurA();
}

int VodoleyModule::runInB()
{
	return MainWindow->CurB();
}

int VodoleyModule::runInC()
{
	return MainWindow->CurC();
}

void VodoleyModule::openRecent()
{
	QAction *s = qobject_cast<QAction *>(sender());
	QString txt = s->text();

	if (!MainWindow->loadFile(txt)) {
		QMessageBox::information(mainWidget(), "", QString::fromUtf8("Ошибка открытия файла! ") + txt, 0, 0, 0);
	}
}

void VodoleyModule::updateLastFiles(QString newFile)
{
	QStringList lastFiles = mySettings()->value("Vodoley/LastFiles").toString().split(";");
	if (lastFiles.indexOf(newFile) < 0) {
		lastFiles.prepend(newFile);
	}
	int max_fid = std::min(int(lastFiles.count()), 11);
	QString sett = "";
	for (int i = 0; i < max_fid; i++) {
		sett += lastFiles.at(i) + ";";
	}
	mySettings()->setValue("Vodoley/LastFiles", sett);
	createRecentMenu();
}

void VodoleyModule::createRecentMenu()
{
	recentMenu->clear();
	QStringList lastFiles = mySettings()->value("Vodoley/LastFiles").toString().split(";");
	qDebug() << lastFiles;
	if (lastFiles.count() == 0 || (lastFiles.count() == 1 && lastFiles[0] == "")) {
		recentMenu->setEnabled(false);
	} else {
		recentMenu->setEnabled(true);
	}

	for (int i = 0; i < lastFiles.count(); i++) {
		if (lastFiles[i] == "") {
			continue;
		}
		QAction *action = recentMenu->addAction(lastFiles[i], this, SLOT(openRecent()));
		Q_UNUSED(action);
	}
}


} // namespace ActorVodoley
