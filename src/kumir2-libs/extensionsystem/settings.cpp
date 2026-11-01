#include "settings.h"

#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#if QT_VERSION >= 0x050000
# include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace ExtensionSystem {

Settings::Settings(const QString & pluginName) :
	pluginName_(pluginName),
	mutex_(new QMutex)
{
#if QT_VERSION >= 0x060000
	// Qt5 DataLocation was the same as AppLocalDataLocation
	static const QString DataLocation = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).at(0);
#elif QT_VERSION >= 0x050000
	static const QString DataLocation = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
#else
	static const QString DataLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
	QString scope = defaultSettingsScope();
	QString fileName = pluginName + ".conf";

	qDebug() << "AppName: ["  << scope << "], PluginName: [" << pluginName << "]";

#ifdef Q_OS_WIN32
	QSettings *sett = new QSettings(DataLocation+"/"+fileName, QSettings::IniFormat);
#else
	QSettings *sett = new QSettings(scope, pluginName);
#endif
	qsettings_.reset(sett);
#if QT_VERSION < 0x060000
	qsettings_->setIniCodec("UTF-8"); // Qt6 always uses UTF-8 for INI files
#endif
	settingsFile_ = qsettings_->fileName();
}

void Settings::setValue(const QString &key, const QVariant &value_)
{
	if (!mutex_)
		return;
	mutex_->lock();
	qsettings_->setValue(key, value_);
	mutex_->unlock();
}

QVariant Settings::value(const QString &key, const QVariant &default_) const
{
	if (!mutex_)
		return QVariant();
	QVariant result;
	mutex_->lock();
	result = qsettings_->value(key, default_);
	mutex_->unlock();
	return result;
}

void Settings::flush()
{
	mutex_->lock();
	qsettings_->sync();
	mutex_->unlock();
}

Settings::~Settings()
{
	flush();
}

void Settings::changeWorkingDirectory(const QString &workDirPath)
{
	workDirPath_ = workDirPath;
	mutex_->lock();
	qsettings_->sync();

	QDir workDir(workDirPath);
	workDir.mkdir(".settings");

	const QString settingsFileName =
			workDirPath + "/.settings/" + pluginName_ + ".conf";
	QSettings * sett = new QSettings(settingsFileName, QSettings::IniFormat);
#if QT_VERSION < 0x060000
	sett->setIniCodec("UTF-8"); // Qt6 always uses UTF-8 for INI files
#endif

	qsettings_.reset(sett);
	settingsFile_ = qsettings_->fileName();

	mutex_->unlock();
}

QString Settings::locationDirectory() const
{
	mutex_->lock();
	const QFileInfo fileInfo(qsettings_->fileName());
	mutex_->unlock();
	return fileInfo.absoluteDir().absolutePath();
}

QString Settings::settingsFilePath() const
{
	mutex_->lock();
	const QString result = settingsFile_;
	mutex_->unlock();
	return result;
}

QString defaultSettingsScope()
{
#ifdef Q_OS_MAC
	static const QString result = "kumir2";
	return result;
#else
	QString appLauncher = QDir::fromNativeSeparators(qApp->arguments().at(0));
	QString appName = QFileInfo(appLauncher).fileName();

	if (appName.endsWith(".exe")) {
		appName = appName.left(appName.length() - 4);
	}

	if (appName.trimmed().isEmpty()) {
		appName = "kumir2";
	}

	return appName;
#endif
}

} // namespace ExtensionSystem
