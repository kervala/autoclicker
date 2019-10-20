/*
 *  AutoClicker is a tool to click automatically
 *  Copyright (C) 2017-2019  Cedric OCHS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common.h"
#include "configfile.h"
#include "moc_configfile.cpp"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

#define IMPLEMENT_SIMPLE_VAR(type, function, var) \
void ConfigFile::set##function(type var)\
{\
	if (m_##var == var) return;\
	\
	m_##var = var;\
	modified(true);\
}\
\
type ConfigFile::get##function() const\
{\
	return m_##var;\
}

#define IMPLEMENT_TYPED_VAR(type, function, var) \
void ConfigFile::set##function(const type &var)\
{\
	if (m_##var == var) return;\
	\
	m_##var = var;\
	modified(true);\
}\
\
type ConfigFile::get##function() const\
{\
	return m_##var;\
}

#define IMPLEMENT_QSTRING_VAR(function, var) \
IMPLEMENT_TYPED_VAR(QString, function, var)

#define IMPLEMENT_QCOLOR_VAR(function, var) \
IMPLEMENT_TYPED_VAR(QColor, function, var)

#define IMPLEMENT_INT_VAR(function, var) \
IMPLEMENT_SIMPLE_VAR(int, function, var)

#define IMPLEMENT_BOOL_VAR(function, var) \
IMPLEMENT_SIMPLE_VAR(bool, function, var)

ConfigFile* ConfigFile::s_instance = NULL;

ConfigFile::ConfigFile(QObject* parent):QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName())
{
	if (!s_instance) s_instance = this;

	initDirectories();
	load();
}

ConfigFile::~ConfigFile()
{
	save();

	s_instance = NULL;
}

bool ConfigFile::load()
{
	int version = m_settings.value("version", 1).toInt();

	if (version == 1)
	{
		loadVersion1();
	}

	return true;
}

bool ConfigFile::loadVersion1()
{
	// window parameters
	m_settings.beginGroup("window");

	m_size = QSize(m_settings.value("width", 0).toInt(), m_settings.value("height", 0).toInt());
	m_position = QPoint(m_settings.value("x", 0).toInt(), m_settings.value("y", 0).toInt());

	m_settings.endGroup();

	updateSettings();

	return true;
}

bool ConfigFile::save()
{
	// no need to save because no change has been made
	if (!m_modified)
	{
		return true;
	}

	// clear previous entries
	m_settings.clear();

	// general parameters
	m_settings.setValue("version", 1);

	// window parameters
	m_settings.beginGroup("window");

	m_settings.setValue("width", m_size.width());
	m_settings.setValue("height", m_size.height());
	m_settings.setValue("x", m_position.x());
	m_settings.setValue("y", m_position.y());

	m_settings.endGroup();

	modified(false);

	return true;
}

QSize ConfigFile::getWindowSize() const
{
	return m_size;
}

void ConfigFile::setWindowSize(const QSize &size)
{
	if (m_size == size || size.width() < 10 || size.height() < 10) return;

	m_size = size;
	modified(true);
}

QPoint ConfigFile::getWindowPosition() const
{
	return m_position;
}

void ConfigFile::setWindowPosition(const QPoint &pos)
{
	if (m_position == pos || pos.isNull()) return;

	m_position = pos;
	modified(true);
}

void ConfigFile::initDirectories()
{
	// logs directory
	QString documentsPath;

#ifdef USE_QT5
	documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
	documentsPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

	// define default logs directory
	m_defaultLogsDirectory = QString("%1/kdAmn/logs").arg(documentsPath);

	if (!QDir().mkpath(m_defaultLogsDirectory))
	{
		qCritical() << "Unable to create directory" << m_defaultLogsDirectory;
	}

	QDir applicationDir(QApplication::applicationDirPath());

	// global data directory
#if defined(Q_OS_WIN32)
#ifdef _DEBUG
	applicationDir.cdUp();
	applicationDir.cdUp();

	// translations are in build directory
	QString buildDirectory = applicationDir.absolutePath();

	// all files are in misc subfolder
	applicationDir.cdUp();
	applicationDir.cd("misc");
#endif
	// same directory as executable
	m_globalDataDirectory = applicationDir.absolutePath();
#else

#ifdef Q_OS_MAC
	applicationDir.cdUp();
	m_globalDataDirectory = applicationDir.absolutePath() + "/Resources";
#else
	// Linux
	QStringList directoriesToSearch;

#ifdef SHARE_PREFIX
	directoriesToSearch << SHARE_PREFIX;
#endif

	// application directory
	directoriesToSearch << applicationDir.absolutePath();

	// application parent directory
	applicationDir.cdUp();
	directoriesToSearch << applicationDir.absolutePath();

	// application share/kdamn directory
	directoriesToSearch << QString("%1/share/%2").arg(applicationDir.absolutePath()).arg(TARGET);;

	// current path
	directoriesToSearch << QDir::currentPath();

	foreach(const QString &directory, directoriesToSearch)
	{
		if (QFile::exists(QString("%1/oembed.ini").arg(directory)))
		{
			m_globalDataDirectory = directory;
			break;
		}
	}

	if (m_globalDataDirectory.isEmpty())
	{
		qDebug() << "Unable to find a directory with resources!";
	}
#endif

#endif

	// translations directory
#if defined(Q_OS_WIN32) && defined(_DEBUG)
	m_translationsDirectory = buildDirectory + "/translations";
#else
	m_translationsDirectory = m_globalDataDirectory + "/translations";
#endif

	// Qt translations directory
#if defined(Q_OS_WIN32)
	m_qtTranslationsDirectory = m_translationsDirectory;
#elif defined(Q_OS_MAC)
	m_qtTranslationsDirectory = m_translationsDirectory;
#elif defined(USE_QT5)
	m_qtTranslationsDirectory = "/usr/share/qt5/translations";
#else
	m_qtTranslationsDirectory = "/usr/share/qt4/translations";
#endif

	// cache directory
#ifdef USE_QT5
	m_cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
	m_cacheDirectory = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

	// local data
#ifdef USE_QT5
	QStandardPaths::StandardLocation location;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	location = QStandardPaths::AppDataLocation;
#else
	location = QStandardPaths::ConfigLocation;
#endif
	m_localDataDirectory = QStandardPaths::writableLocation(location);
#else
	m_localDataDirectory = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

	// download directory
#ifdef USE_QT5
	m_downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#else
	m_downloadDirectory = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

	// create directories
	QDir().mkpath(m_localDataDirectory);
}

void ConfigFile::updateSettings()
{
	// logs directory
	if (m_logsDirectory.isEmpty() || !QFile::exists(m_logsDirectory))
	{
		m_logsDirectory = m_defaultLogsDirectory;
	}
}

void ConfigFile::modified(bool modified)
{
	m_modified = modified;
}

IMPLEMENT_QSTRING_VAR(DefaultLogsDirectory, defaultLogsDirectory);
IMPLEMENT_QSTRING_VAR(LogsDirectory, logsDirectory);
IMPLEMENT_QSTRING_VAR(TranslationsDirectory, translationsDirectory);
IMPLEMENT_QSTRING_VAR(QtTranslationsDirectory, qtTranslationsDirectory);
IMPLEMENT_QSTRING_VAR(CacheDirectory, cacheDirectory);
IMPLEMENT_QSTRING_VAR(DownloadDirectory, downloadDirectory);
IMPLEMENT_QSTRING_VAR(GlobalDataDirectory, globalDataDirectory);
IMPLEMENT_QSTRING_VAR(LocalDataDirectory, localDataDirectory);
