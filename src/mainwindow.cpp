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

#include "common.h"
#include "mainwindow.h"
#include "moc_mainwindow.cpp"
#include "configfile.h"
#include "configfile.h"
#include "updatedialog.h"
#include "updater.h"
#include "spotmodel.h"

#ifdef Q_OS_WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinTaskbarButton>
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow(nullptr, Qt::WindowStaysOnTopHint)
{
	setupUi(this);

#ifdef Q_OS_WIN32
	m_button = new QWinTaskbarButton(this);
#endif

	// Server menu
	//connect(actionSettings, &QAction::triggered, this, &MainWindow::onSettings);
	connect(actionExit, &QAction::triggered, this, &MainWindow::close);

	// Help menu
	connect(actionCheckUpdates, &QAction::triggered, this, &MainWindow::onCheckUpdates);
	connect(actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
	connect(actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);

	QSize size = ConfigFile::getInstance()->getWindowSize();
	if (!size.isNull()) resize(size);

	QPoint pos = ConfigFile::getInstance()->getWindowPosition();
	if (!pos.isNull()) move(pos);

	// Buttons
	connect(addPushButton, &QPushButton::clicked, this, &MainWindow::onAdd);
	connect(removePushButton, &QPushButton::clicked, this, &MainWindow::onRemove);
	connect(startPushButton, &QPushButton::clicked, this, &MainWindow::onStartOrStop);
	connect(keyPushButton, &QPushButton::clicked, this, &MainWindow::onKey);
	connect(positionPushButton, &QPushButton::clicked, this, &MainWindow::onPosition);

	// Systray
	SystrayIcon *systray = new SystrayIcon(this);
	connect(systray, &SystrayIcon::requestMinimize, this, &MainWindow::onMinimize);
	connect(systray, &SystrayIcon::requestRestore, this, &MainWindow::onRestore);
	connect(systray, &SystrayIcon::requestClose, this, &MainWindow::close);
	connect(systray, &SystrayIcon::requestAction, this, &MainWindow::onSystrayAction);

	m_model = new SpotModel(this);

	spotsListView->setModel(m_model);

	connect(this, &MainWindow::mousePosition, this, &MainWindow::onMousePositionChanged);

	// check for a new version
	Updater *updater = new Updater(this);
	connect(updater, &Updater::newVersionDetected, this, &MainWindow::onNewVersion);
	updater->checkUpdates();

	spotGroupBox->setVisible(false);

	m_mapper = new QDataWidgetMapper(this);
	m_mapper->setModel(m_model);
	m_mapper->addMapping(nameLineEdit, 0);
	m_mapper->addMapping(positionPushButton, 1);
	m_mapper->addMapping(delaySpinBox, 2);

	connect(spotsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onSelectionChanged);
	connect(spotsListView->selectionModel(), &QItemSelectionModel::currentRowChanged, m_mapper, &QDataWidgetMapper::setCurrentModelIndex);
}

MainWindow::~MainWindow()
{
}

void MainWindow::showEvent(QShowEvent *e)
{
#ifdef Q_OS_WIN32
	m_button->setWindow(windowHandle());
#endif

	e->accept();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	hide();

	e->accept();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
	ConfigFile::getInstance()->setWindowSize(e->size());

	e->accept();
}

void MainWindow::moveEvent(QMoveEvent *e)
{
	ConfigFile::getInstance()->setWindowPosition(QPoint(x(), y()));

	e->accept();
}

void MainWindow::onAdd()
{
	QModelIndexList indices = spotsListView->selectionModel()->selectedRows();

	int row = indices.isEmpty() ? -1:indices.front().row();

	m_model->insertRow(row);
}

void MainWindow::onRemove()
{
	QModelIndexList indices = spotsListView->selectionModel()->selectedRows();

	if (indices.isEmpty()) return;

	int row = indices.front().row();

	m_model->removeRow(row);

}

void MainWindow::onStartOrStop()
{
	for (int i = 0; i < m_timers.size(); ++i)
	{
		m_timers[i]->stop();
		m_timers[i]->deleteLater();
		m_timers[i] = nullptr;
	}

	m_timers.clear();

	if (startPushButton->text() == tr("Stop"))
	{
		startPushButton->setText(tr("Start"));
		return;
	}

	for (int row = 0; row < m_model->rowCount(); ++row)
	{
		QTimer* timer = new QTimer(this);
		timer->setProperty("row", row);
		timer->setSingleShot(true);
		
		// wait 1 second before to really start
		timer->setInterval(1000);

		connect(timer, &QTimer::timeout, this, &MainWindow::onTimer);

		m_timers.push_back(timer);
	}

	m_lastMousePosition = QCursor::pos();

	// start all timers
	for (int i = 0; i < m_timers.size(); ++i)
	{
		m_timers[i]->start();
	}

	startPushButton->setText(tr("Stop"));
}

void MainWindow::onTimer()
{
	QTimer* timer = qobject_cast<QTimer*>(sender());

	int row = timer->property("row").toInt();

	Spot spot = m_model->getSpot(row);

	// stop auto-click if move the mouse
	if (QCursor::pos() != m_lastMousePosition)
	{
		onStartOrStop();
	}

	// 50% change position
	if (QRandomGenerator::global()->bounded(0, 1) == 0)
	{
		// randomize position
		int dx = QRandomGenerator::global()->bounded(0, 2) - 1;
		int dy = QRandomGenerator::global()->bounded(0, 2) - 1;

		// invert sign
		if ((spot.lastPosition.x() + dx > (spot.originalPosition.x() + 5)) || (spot.lastPosition.x() + dx < (spot.originalPosition.x() - 5))) dx = -dx;
		if ((spot.lastPosition.y() + dy > (spot.originalPosition.y() + 5)) || (spot.lastPosition.y() + dx < (spot.originalPosition.y() - 5))) dy = -dy;

		spot.lastPosition += QPoint(dx, dy);
	}

	// set cursor position
	SetCursorPos(spot.lastPosition.x(), spot.lastPosition.y());

	// update last mouse position
	m_lastMousePosition = spot.lastPosition;

	// left click down
	mouse_event(MOUSEEVENTF_LEFTDOWN, spot.lastPosition.x(), spot.lastPosition.y(), 0, 0);

	// between 6 and 14 clicks/second = 125-166

	// wait a little before releasing the mouse
	QThread::currentThread()->msleep(QRandomGenerator::global()->bounded(10, 25));

	// left click up
	mouse_event(MOUSEEVENTF_LEFTUP, spot.lastPosition.x(), spot.lastPosition.y(), 0, 0);

	int delay = QRandomGenerator::global()->bounded(30, spot.delay);

	qDebug() << "timeout" << row << "delay" << delay;

	timer->setInterval(delay);
	timer->start();
}

void MainWindow::onKey()
{
}

void MainWindow::onPosition()
{
	QModelIndex index = spotsListView->selectionModel()->currentIndex();

	positionPushButton->setEnabled(false);
	positionPushButton->setText("???");

	QFuture<void> future = QtConcurrent::run(this, &MainWindow::getMousePosition);
}

void MainWindow::getMousePosition()
{
	QPoint default(0, 0);

	m_mousePosition = default;

	while (m_mousePosition == default && QThread::currentThread()->isRunning())
	{
		QThread::currentThread()->msleep(100);
	}

	emit mousePosition(m_mousePosition);
}

void MainWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	// update controls
	spotGroupBox->setVisible(!selected.empty());

	if (selected.empty()) return;

	// update controls
	const QItemSelectionRange &range = selected.front();

	QModelIndexList indices = range.indexes();

	int row = indices.front().row();

	// manually update position because not handled by data mapper
	QPoint pos = m_model->getSpot(row).originalPosition;

	positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void MainWindow::onMousePositionChanged(const QPoint& pos)
{
	QModelIndex index = spotsListView->selectionModel()->currentIndex();

	// update original and last positions
	m_model->setData(m_model->index(index.row(), 1), pos);
	m_model->setData(m_model->index(index.row(), 3), pos);

	positionPushButton->setEnabled(true);
	positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void MainWindow::onCheckUpdates()
{
	Updater *updater = new Updater(this);
	connect(updater, SIGNAL(newVersionDetected(QString, QString, uint, QString)), this, SLOT(onNewVersion(QString, QString, uint, QString)));
	connect(updater, SIGNAL(noNewVersionDetected()), this, SLOT(onNoNewVersion()));
	updater->checkUpdates();
}

void MainWindow::onAbout()
{
	QMessageBox::about(this,
		tr("About %1").arg(QApplication::applicationName()),
		QString("%1 %2<br>").arg(QApplication::applicationName()).arg(QApplication::applicationVersion())+
		tr("deviantART Messaging Network (dAmn) chat client")+
		QString("<br><br>")+
		tr("Author: %1").arg("<a href=\"http://kervala.deviantart.com\">Kervala</a><br>")+
		tr("Support: %1").arg("<a href=\"http://dev.kervala.net/projects/autoclicker\">http://dev.kervala.net/projects/autoclicker</a>"));
}

void MainWindow::onAboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::onMinimize()
{
	// only hide window if using systray and enabled hide minized window
	if (isVisible() && ConfigFile::getInstance()->getUseSystray() && ConfigFile::getInstance()->getHideMinimizedWindow())
	{
		hide();
	}
}

void MainWindow::onRestore()
{
	if (!isVisible())
	{
		if (isMaximized())
		{
			showMaximized();
		}
		else if (isMinimized())
		{
			showNormal();
		}
	}

	raise();
	activateWindow();
}

void MainWindow::onSystrayAction(SystrayIcon::SystrayAction action)
{
	switch(action)
	{
		case SystrayIcon::ActionOpenURL:
		//if (!m_urlToCheck.isEmpty()) QDesktopServices::openUrl(m_urlToCheck);
		break;

		case SystrayIcon::ActionReadLastNote:
		//QDesktopServices::openUrl(QUrl("https://www.deviantart.com/messages/notes/"));
		break;

		default:
		break;
	}
}

bool MainWindow::event(QEvent *e)
{
	if (e->type() == QEvent::WindowActivate)
	{
	}
	else if (e->type() == QEvent::WindowDeactivate)
	{
		// click outside application
		m_mousePosition = QCursor::pos();
	}
	else if (e->type() == QEvent::LanguageChange)
	{
		retranslateUi(this);
	}
	else if (e->type() == QEvent::WindowStateChange)
	{
		if (windowState() & Qt::WindowMinimized)
		{
			QTimer::singleShot(250, this, SLOT(onMinimize()));
		}
	}

	return QMainWindow::event(e);
}

void MainWindow::onNewVersion(const QString &url, const QString &date, uint size, const QString &version)
{
	QMessageBox::StandardButton reply = QMessageBox::question(this,
		tr("New version"),
		tr("Version %1 is available since %2.\n\nDo you want to download it now?").arg(version).arg(date),
		QMessageBox::Yes|QMessageBox::No);

	if (reply != QMessageBox::Yes) return;

	UpdateDialog dialog(this);

	connect(&dialog, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onProgress(qint64, qint64)));

	dialog.download(url, size);

	if (dialog.exec() == QDialog::Accepted)
	{
		// if user clicked on Install, close kdAmn
		close();
	}
}

void MainWindow::onNoNewVersion()
{
	QMessageBox::information(this,
		tr("No update found"),
		tr("You already have the last %1 version (%2).").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()));
}

void MainWindow::onProgress(qint64 readBytes, qint64 totalBytes)
{
#ifdef Q_OS_WIN32
	QWinTaskbarProgress *progress = m_button->progress();

	if (readBytes == totalBytes)
	{
		// end
		progress->hide();
	}
	else if (readBytes == 0)
	{
//		TODO: see why it doesn't work
//		m_button->setOverlayIcon(style()->standardIcon(QStyle::SP_MediaPlay) /* QIcon(":/icons/upload.svg") */);
//		m_button->setOverlayAccessibleDescription(tr("Upload"));

		// beginning
		progress->show();
		progress->setRange(0, totalBytes);
	}
	else
	{
		progress->show();
		progress->setValue(readBytes);
	}
#else
	// TODO: for other OSes
#endif
}
