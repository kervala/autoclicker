/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2015  Cedric OCHS
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "systrayicon.h"

class QWinTaskbarButton;
class SpotModel;
class QDataWidgetMapper;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	MainWindow();
	virtual ~MainWindow();

public slots:
	// buttons

	// actions
	void onLoad();
	void onSave();
	void onStartOrStop();

	// spots
	void onAdd();
	void onRemove();
	void onPosition();

	// file menu

	// help menu
	void onCheckUpdates();
	void onAbout();
	void onAboutQt();

	// other
	void onMinimize();
	void onRestore();
	void onSystrayAction(SystrayIcon::SystrayAction action);

	// signals from OAuth2
	void onNewVersion(const QString &url, const QString &date, uint size, const QString &version);
	void onNoNewVersion();
	void onProgress(qint64 readBytes, qint64 totalBytes);

	void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void onMousePositionChanged(const QPoint& pos);

	void onTimer();

signals:
	void mousePosition(const QPoint &pos);

protected:
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *e);
	void resizeEvent(QResizeEvent *e);
	void moveEvent(QMoveEvent *e);
	bool event(QEvent *e);

	void getMousePosition();

	QWinTaskbarButton *m_button;
	SpotModel *m_model;
	QDataWidgetMapper *m_mapper;
	QPoint m_mousePosition;
	QPoint m_lastMousePosition;

	QList<QTimer*> m_timers;
};

#endif