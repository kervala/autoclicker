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
#include "testdialog.h"
#include "moc_testdialog.cpp"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

TestDialog::TestDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint), m_clicks(0), m_minClickDelay(0.0), m_maxClickDelay(0.0)
{
	setupUi(this);

	setMouseTracking(true);

	reset();
}

TestDialog::~TestDialog()
{
}

void TestDialog::reset()
{
	m_firstPress = QDateTime();
	m_lastPress = QDateTime();
	m_lastRelease = QDateTime();

	m_clicks = 0;
	m_minClickDelay = 0.0;
	m_maxClickDelay = 0.0;
}

void TestDialog::mousePressEvent(QMouseEvent* /* event */)
{
	QDateTime now = QDateTime::currentDateTime();

	if (m_firstPress.isNull()) m_firstPress = now;

	m_lastPress = now;
}

void TestDialog::mouseReleaseEvent(QMouseEvent* /* event */)
{
	QDateTime now = QDateTime::currentDateTime();

	++m_clicks;

	qint64 lastClickDelay = m_lastRelease.msecsTo(now);

	if (m_minClickDelay == 0.0 || lastClickDelay < m_minClickDelay) m_minClickDelay = lastClickDelay;
	if (m_maxClickDelay == 0.0 || lastClickDelay > m_maxClickDelay) m_maxClickDelay = lastClickDelay;

	qint64 clicksDuration = m_firstPress.msecsTo(now); // total ms
	double averageCPS = (double)m_clicks / (double)clicksDuration * 1000.0;
	double averageClicksDelay = 1000.0 / averageCPS;

	m_lastRelease = now;

	clicksLabel->setText(tr("Number of clicks: %1").arg(m_clicks));
	releaseLabel->setText(tr("Click delay: %1 ms (average: %2 / min: %3 / max: %4)").arg(lastClickDelay).arg(averageClicksDelay).arg(m_minClickDelay).arg(m_maxClickDelay));
	averageLabel->setText(tr("CPS: %1 cps").arg(averageCPS));
}

void TestDialog::mouseMoveEvent(QMouseEvent* event)
{
	positionLabel->setText(tr("Mouse position: (%1, %2)").arg(event->globalPosition().x()).arg(event->globalPosition().y()));
}

void TestDialog::closeEvent(QCloseEvent* e)
{
	hide();

	e->accept();
}

void TestDialog::resizeEvent(QResizeEvent* e)
{
	ConfigFile::getInstance()->setTestDialogSize(e->size());

	e->accept();
}

void TestDialog::moveEvent(QMoveEvent* e)
{
	ConfigFile::getInstance()->setTestDialogPosition(QPoint(x(), y()));

	e->accept();
}
