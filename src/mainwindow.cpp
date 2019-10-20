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
#include "mainwindow.h"
#include "moc_mainwindow.cpp"
#include "ui_mainwindow.h"
#include "configfile.h"
#include "updatedialog.h"
#include "updater.h"
#include "spotmodel.h"
#include "utils.h"
#include "testdialog.h"

#ifdef Q_OS_WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinTaskbarButton>
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow() : QMainWindow(nullptr, Qt::WindowStaysOnTopHint), m_stopExternalListener(0), m_startShortcut(nullptr)
{
	m_ui = new Ui::MainWindow();
	m_ui->setupUi(this);

#ifdef Q_OS_WIN32
	m_button = new QWinTaskbarButton(this);
#endif

	QSize size = ConfigFile::getInstance()->getWindowSize();
	if (!size.isNull()) resize(size);

	QPoint pos = ConfigFile::getInstance()->getWindowPosition();
	if (!pos.isNull()) move(pos);

	SystrayIcon *systray = new SystrayIcon(this);

	m_model = new SpotModel(this);

	m_ui->spotsListView->setModel(m_model);

	// check for a new version
	m_updater = new Updater(this);

	m_ui->spotGroupBox->setVisible(false);

	m_mapper = new QDataWidgetMapper(this);
	m_mapper->setModel(m_model);
	m_mapper->addMapping(m_ui->nameLineEdit, 0);
	// m_mapper->addMapping(positionPushButton, 1);
	m_mapper->addMapping(m_ui->delaySpinBox, 2);

	m_startShortcut = new QShortcut(QKeySequence(), this);

	// File menu
	connect(m_ui->actionTestDialog, &QAction::triggered, this, &MainWindow::onTestDialog);
	connect(m_ui->actionExit, &QAction::triggered, this, &MainWindow::close);

	// Help menu
	connect(m_ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::onCheckUpdates);
	connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
	connect(m_ui->actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);

	// Buttons
	connect(m_ui->addPushButton, &QPushButton::clicked, this, &MainWindow::onAdd);
	connect(m_ui->removePushButton, &QPushButton::clicked, this, &MainWindow::onRemove);
	connect(m_ui->startPushButton, &QPushButton::clicked, this, &MainWindow::onStartOrStop);
	connect(m_ui->loadPushButton, &QPushButton::clicked, this, &MainWindow::onLoad);
	connect(m_ui->savePushButton, &QPushButton::clicked, this, &MainWindow::onSave);
	connect(m_ui->positionPushButton, &QPushButton::clicked, this, &MainWindow::onPosition);
	connect(m_ui->startKeySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &MainWindow::onStartKeyChanged);

	// Systray
	connect(systray, &SystrayIcon::requestMinimize, this, &MainWindow::onMinimize);
	connect(systray, &SystrayIcon::requestRestore, this, &MainWindow::onRestore);
	connect(systray, &SystrayIcon::requestClose, this, &MainWindow::close);
	connect(systray, &SystrayIcon::requestAction, this, &MainWindow::onSystrayAction);

	// Selection model
	connect(m_ui->spotsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onSelectionChanged);
	connect(m_ui->spotsListView->selectionModel(), &QItemSelectionModel::currentRowChanged, m_mapper, &QDataWidgetMapper::setCurrentModelIndex);

	// MainWindow
	connect(this, &MainWindow::startSimple, this, &MainWindow::onStartSimple);
	connect(this, &MainWindow::mousePosition, this, &MainWindow::onMousePositionChanged);

	// Updater
	connect(m_updater, &Updater::newVersionDetected, this, &MainWindow::onNewVersion);
	connect(m_updater, &Updater::noNewVersionDetected, this, &MainWindow::onNoNewVersion);

	// Shortcut
	connect(m_startShortcut, &QShortcut::activated, this, &MainWindow::onStartSimple);

	m_updater->checkUpdates(true);
}

MainWindow::~MainWindow()
{
	delete m_ui;
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

void MainWindow::onStartKeyChanged(const QKeySequence &keySequence)
{
	m_startShortcut->setKey(keySequence);
}

void MainWindow::onAdd()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

	int row = indices.isEmpty() ? -1:indices.front().row();

	m_model->insertRow(row);
}

void MainWindow::onRemove()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

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

	if (m_ui->startPushButton->text() == tr("Stop"))
	{
		m_ui->startPushButton->setText(tr("Start"));
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

	m_lastMousePosition = QPoint();

	// start all timers
	for (int i = 0; i < m_timers.size(); ++i)
	{
		m_timers[i]->start();
	}

	m_ui->startPushButton->setText(tr("Stop"));
}

static int randomNumber(int min, int max)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	return QRandomGenerator::global()->bounded(min, max);
#else
	return 0;
#endif
}

void MainWindow::onTimer()
{
	// do this because sometimes, the mouse move when clicking
	if (m_lastMousePosition == QPoint()) m_lastMousePosition = QCursor::pos();

	QTimer* timer = qobject_cast<QTimer*>(sender());

	int row = timer->property("row").toInt();

	Spot spot;
	
	if (row < 0)
	{
		// simple mode
		spot = m_spot;
	}
	else
	{
		// multi mode
		spot = m_model->getSpot(row);
	}

	// stop auto-click if move the mouse
	if (QCursor::pos() != m_lastMousePosition)
	{
		onStartOrStop();
	}

	// 50% change position
	if (randomNumber(0, 1) == 0)
	{
		// randomize position
		int dx = randomNumber(0, 2) - 1;
		int dy = randomNumber(0, 2) - 1;

		// invert sign
		if ((spot.lastPosition.x() + dx > (spot.originalPosition.x() + 5)) || (spot.lastPosition.x() + dx < (spot.originalPosition.x() - 5))) dx = -dx;
		if ((spot.lastPosition.y() + dy > (spot.originalPosition.y() + 5)) || (spot.lastPosition.y() + dx < (spot.originalPosition.y() - 5))) dy = -dy;

		spot.lastPosition += QPoint(dx, dy);

		if (row < 0)
		{
			// simple mode
			m_spot = spot;
		}
		else
		{
			// multi mode
			m_model->setSpot(row, spot);
		}
	}

	// set cursor position
	QCursor::setPos(spot.lastPosition);

	// update last mouse position
	m_lastMousePosition = spot.lastPosition;

	// left click down
	mouseLeftClickDown(spot.lastPosition);

	// between 6 and 14 clicks/second = 125-166

	// wait a little before releasing the mouse
	QThread::currentThread()->msleep(randomNumber(10, 25));

	// left click up
	mouseLeftClickUp(spot.lastPosition);

	int delay = randomNumber(30, spot.delay) - 10 /* Qt latency */;

	// restart timer
	timer->start(delay);
}

void MainWindow::onLoad()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load actions"), ConfigFile::getInstance()->getLocalDataDirectory(), "AutoClicker Files (*.acf)");

	if (filename.isEmpty()) return;

	m_model->load(filename);
}

void MainWindow::onSave()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save actions"), ConfigFile::getInstance()->getLocalDataDirectory(), "AutoClicker Files (*.acf)");

	if (filename.isEmpty()) return;

	m_model->save(filename);
}

void MainWindow::onPosition()
{
	QModelIndex index = m_ui->spotsListView->selectionModel()->currentIndex();

	m_ui->positionPushButton->setEnabled(false);
	m_ui->positionPushButton->setText("???");

	QtConcurrent::run(this, &MainWindow::getMousePosition);
}

void MainWindow::onTestDialog()
{
	static TestDialog* s_dialog = nullptr;

	if (!s_dialog)
	{
		s_dialog = new TestDialog(this);
		s_dialog->setModal(false);
	}

	s_dialog->reset();
	s_dialog->show();
}

void MainWindow::getMousePosition()
{
	QPoint defaultPosition(0, 0);

	m_mousePosition = defaultPosition;

	while (m_mousePosition == defaultPosition && QThread::currentThread()->isRunning())
	{
		QThread::currentThread()->msleep(100);
	}

	emit mousePosition(m_mousePosition);
}

void MainWindow::listenExternalInputEvents()
{
	QKeySequence sequence = m_ui->startKeySequenceEdit->keySequence();

	qint16 key = QKeySequenceToVK(sequence);

	// no key defined
	if (key == 0) return;

	while (!m_stopExternalListener)
	{
		bool buttonPressed = isKeyPressed(key);

		if (buttonPressed)
		{
			qDebug() << "key" << sequence << "pressed (" << key << ")";

			emit startSimple();
		}

		QThread::currentThread()->msleep(10);
	}
}

void MainWindow::onStartSimple()
{
	for (int i = 0; i < m_timers.size(); ++i)
	{
		m_timers[i]->stop();
		m_timers[i]->deleteLater();
		m_timers[i] = nullptr;
	}

	m_timers.clear();

	if (m_ui->startPushButton->text() == tr("Stop"))
	{
		m_ui->startPushButton->setText(tr("Start"));
		return;
	}

	QTimer* timer = new QTimer(this);
	timer->setProperty("row", -1);
	timer->setSingleShot(true);

	// wait 1 second before to really start
	timer->setInterval(100);

	connect(timer, &QTimer::timeout, this, &MainWindow::onTimer);

	m_timers << timer;

	m_spot.delay = m_ui->defaultDelaySpinBox->value();
	m_spot.lastPosition = QCursor::pos();
	m_spot.originalPosition = m_spot.lastPosition;
	m_lastMousePosition = m_spot.lastPosition;

	// start all timers
	m_timers[0]->start();

	m_ui->startPushButton->setText(tr("Stop"));
}

void MainWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	// update controls
	m_ui->spotGroupBox->setVisible(!selected.empty());

	if (selected.empty()) return;

	// update controls
	const QItemSelectionRange &range = selected.front();

	QModelIndexList indices = range.indexes();

	int row = indices.front().row();

	// manually update position because not handled by data mapper
	QPoint pos = m_model->getSpot(row).originalPosition;

	m_ui->positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void MainWindow::onMousePositionChanged(const QPoint& pos)
{
	QModelIndex index = m_ui->spotsListView->selectionModel()->currentIndex();

	// update original and last positions
	m_model->setData(m_model->index(index.row(), 1), pos);
	m_model->setData(m_model->index(index.row(), 3), pos);

	m_ui->positionPushButton->setEnabled(true);
	m_ui->positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void MainWindow::onCheckUpdates()
{
	m_updater->checkUpdates(false);
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
	if (isVisible())
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
		m_stopExternalListener = true;
	}
	else if (e->type() == QEvent::WindowDeactivate)
	{
		if (!isHidden())
		{
			// click outside application
			m_mousePosition = QCursor::pos();

			m_stopExternalListener = false;

			QtConcurrent::run(this, &MainWindow::listenExternalInputEvents);
		}
	}
	else if (e->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
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

	connect(&dialog, &UpdateDialog::downloadProgress, this, &MainWindow::onProgress);

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
