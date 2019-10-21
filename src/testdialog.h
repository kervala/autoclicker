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

#ifndef TESTDIALOG_H
#define TESTDIALOG_H

#include "ui_testdialog.h"

class TestDialog : public QDialog, public Ui::TestDialog
{
	Q_OBJECT

public:
	TestDialog(QWidget* parent);
	virtual ~TestDialog();

	void reset();

private:
	QDateTime m_firstPress;
	QDateTime m_lastPress;
	QDateTime m_lastRelease;
	qint64 m_clicks;
	double m_minClickDelay;
	double m_maxClickDelay;

protected:
	// mouse events
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);

	// dialog events
	void closeEvent(QCloseEvent* e);
	void resizeEvent(QResizeEvent* e);
	void moveEvent(QMoveEvent* e);
};

#endif
