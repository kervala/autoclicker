/*
 *  kClicker is a tool to click automatically
 *  Copyright (C) 2017-2022  Cedric OCHS
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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#define DECLARE_SIMPLE_VAR(type, function, var) \
public:\
	void set##function(type var);\
	type get##function() const;\
protected:\
	type m_##var;

#define DECLARE_TYPED_VAR(type, function, var) \
public:\
	void set##function(const type &var);\
	type get##function() const;\
protected:\
	type m_##var;

#define DECLARE_QSTRING_VAR(function, var) \
DECLARE_TYPED_VAR(QString, function, var);

#define DECLARE_QCOLOR_VAR(function, var) \
DECLARE_TYPED_VAR(QColor, function, var);

#define DECLARE_INT_VAR(function, var) \
DECLARE_SIMPLE_VAR(int, function, var);

#define DECLARE_BOOL_VAR(function, var) \
DECLARE_SIMPLE_VAR(bool, function, var);

class ConfigFile : public QObject
{
	Q_OBJECT

public:
	ConfigFile(QObject* parent = NULL);
	virtual ~ConfigFile();

	static ConfigFile* getInstance() { return s_instance; }

	void updateSettings();

DECLARE_QSTRING_VAR(DefaultLogsDirectory, defaultLogsDirectory);
DECLARE_QSTRING_VAR(LogsDirectory, logsDirectory);
DECLARE_QSTRING_VAR(TranslationsDirectory, translationsDirectory);
DECLARE_QSTRING_VAR(QtTranslationsDirectory, qtTranslationsDirectory);
DECLARE_QSTRING_VAR(CacheDirectory, cacheDirectory);
DECLARE_QSTRING_VAR(DownloadDirectory, downloadDirectory);
DECLARE_QSTRING_VAR(GlobalDataDirectory, globalDataDirectory);
DECLARE_QSTRING_VAR(LocalDataDirectory, localDataDirectory);
DECLARE_QSTRING_VAR(StartKey, startKey);
DECLARE_TYPED_VAR(QSize, WindowSize, size);
DECLARE_TYPED_VAR(QPoint, WindowPosition, position);
DECLARE_TYPED_VAR(QSize, TestDialogSize, testDialogSize);
DECLARE_TYPED_VAR(QPoint, TestDialogPosition, testDialogPosition);
DECLARE_INT_VAR(Delay, delay);

public slots:
	bool load();
	bool save();

private:
	void initDirectories();
	void modified(bool modified);

	static ConfigFile* s_instance;

	bool loadVersion1();

	QSettings m_settings;

	bool m_modified;
};

#endif
