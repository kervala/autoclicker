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
#include "editscriptdialog.h"
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

MainWindow::MainWindow() : QMainWindow(nullptr, Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint), m_stopExternalListener(0), m_stopClicker(0), m_useSimpleMode(true)
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

	m_model = new ActionModel(this);
	// check for a new version
	m_updater = new Updater(this);

	m_ui->startKeySequenceEdit->setKeySequence(QKeySequence(ConfigFile::getInstance()->getStartKey()));
	m_ui->positionKeySequenceEdit->setKeySequence(QKeySequence(ConfigFile::getInstance()->getPositionKey()));
	m_ui->defaultDelaySpinBox->setValue(ConfigFile::getInstance()->getDelay());

	// File menu
	connect(m_ui->actionNew, &QAction::triggered, this, &MainWindow::onNew);
	connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpen);
	connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow::onSave);
	connect(m_ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
	connect(m_ui->actionTestDialog, &QAction::triggered, this, &MainWindow::onTestDialog);
	connect(m_ui->actionExit, &QAction::triggered, this, &MainWindow::close);

	// Help menu
	connect(m_ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::onCheckUpdates);
	connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
	connect(m_ui->actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);

	// Buttons
	connect(m_ui->editPushButton, &QPushButton::clicked, this, &MainWindow::onEditScript);
	connect(m_ui->startPushButton, &QPushButton::clicked, this, &MainWindow::onStartOrStop);

	// Keys
	connect(m_ui->startKeySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &MainWindow::onStartKeyChanged);
	connect(m_ui->positionKeySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &MainWindow::onPositionKeyChanged);

	connect(m_ui->defaultDelaySpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::onDelayChanged);

	// Systray
	connect(systray, &SystrayIcon::requestMinimize, this, &MainWindow::onMinimize);
	connect(systray, &SystrayIcon::requestRestore, this, &MainWindow::onRestore);
	connect(systray, &SystrayIcon::requestClose, this, &MainWindow::close);
	connect(systray, &SystrayIcon::requestAction, this, &MainWindow::onSystrayAction);

	// MainWindow
	connect(this, &MainWindow::startSimple, this, &MainWindow::onStartSimple);
	connect(this, &MainWindow::clickerStopped, this, &MainWindow::onStartOrStop);
	connect(this, &MainWindow::changeSystrayIcon, this, &MainWindow::onChangeSystrayIcon);

	// Updater
	connect(m_updater, &Updater::newVersionDetected, this, &MainWindow::onNewVersion);
	connect(m_updater, &Updater::noNewVersionDetected, this, &MainWindow::onNoNewVersion);

	m_updater->checkUpdates(true);

	startListeningExternalInputEvents();
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
	m_stopExternalListener = 1;

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

void MainWindow::startOrStop(bool simpleMode)
{
	// stop clicker is greater than 0 when stopped
	if (m_stopClicker)
	{
		m_stopClicker = 0;

		show();

		SystrayIcon::getInstance()->setStatus(SystrayIcon::StatusNormal);

		m_ui->startPushButton->setText(tr("Start"));

		// listen again for external input
		startListeningExternalInputEvents();

		return;
	}

	m_ui->startPushButton->setText(tr("Stop"));

	hide();

	// reset stop flag
	m_stopClicker = 0;
	m_useSimpleMode = simpleMode;

	QtConcurrent::run(this, &MainWindow::clicker);
}

void MainWindow::updateStartButton()
{
	m_ui->startPushButton->setEnabled(m_model->rowCount() > 0);
}

void MainWindow::onEditScript()
{
	EditScriptDialog dialog(this, m_model);

	if (dialog.exec() == QDialog::Accepted)
	{
		// delete current model
		delete m_model;

		// use the new model
		m_model = dialog.getModel()->clone(this);

		updateStartButton();
	}
}

void MainWindow::onStartOrStop()
{
	startOrStop(false);
}

void MainWindow::onStartSimple()
{
	// define unique spot parameters
	m_action.delay = m_ui->defaultDelaySpinBox->value();
	m_action.lastPosition = QCursor::pos();
	m_action.originalPosition = m_action.lastPosition;
	m_action.originalCount = m_action.lastCount;

	startOrStop(true);
}

void MainWindow::onChangeSystrayIcon()
{
	SystrayIcon::SystrayStatus status = SystrayIcon::getInstance()->getStatus() == SystrayIcon::StatusClick ? SystrayIcon::StatusNormal : SystrayIcon::StatusClick;

	SystrayIcon::getInstance()->setStatus(status);
}

static int randomNumber(int min, int max)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
	// max can't be less or equal to min
	return QRandomGenerator::global()->bounded(min, qMax(min + 1, max));
#else
	return 0;
#endif
}

void MainWindow::clicker()
{
	QRect rect;

	// wait a little
	if (!m_useSimpleMode) QThread::currentThread()->sleep(1);

	int row = 0;

	Action action;
	QTime startTime = QTime::currentTime();
	QTime endTime;

	if (m_useSimpleMode)
	{
		// simple mode
		action = m_action;

		// always use absolute coordinates
		rect = QRect(0, 0, 10, 10);
	}
	else
	{
		QString title = m_model->getWindowTitle();
		Window window;

		// reset repeat count
		m_model->resetCount();

		if (!title.isEmpty())
		{
			Windows windows;
			createWindowsList(windows);

			for (int i = 0; i < windows.size(); ++i)
			{
				if (windows[i].title == title)
				{
					window = windows[i];
					break;
				}
			}

			rect = window.rect;
		}
		else
		{
			// only top left position is used
			rect = QRect(0, 0, 10, 10);
		}

		// no window with that name
		if (rect.isNull())
		{
			m_stopClicker = 1;
		}
		else
		{
			// multi mode
			action = m_model->getAction(row);

			if (window.id)
			{
			}

			// apply window offset
			action.originalPosition += rect.topLeft();
			action.lastPosition = action.originalPosition;

			if (!isSameWindowAtPos(window, action.originalPosition))
			{
				m_stopClicker = 1;
			}
		}
	}

	while(!m_stopClicker)
	{
		if (action.type == TypeClick)
		{
			// 50% change position
			if (randomNumber(0, 1) == 0)
			{
				// randomize position
				int dx = randomNumber(0, 2) - 1;
				int dy = randomNumber(0, 2) - 1;

				// invert sign
				if ((action.lastPosition.x() + dx > (action.originalPosition.x() + 5)) || (action.lastPosition.x() + dx < (action.originalPosition.x() - 5))) dx = -dx;
				if ((action.lastPosition.y() + dy > (action.originalPosition.y() + 5)) || (action.lastPosition.y() + dx < (action.originalPosition.y() - 5))) dy = -dy;

				action.lastPosition += QPoint(dx, dy);
			}

			// set cursor position
			QCursor::setPos(action.lastPosition);

			// left click down
			mouseLeftClickDown(action.lastPosition);

			// between 6 and 14 clicks/second = 125-166

			// wait a little before releasing the mouse
			QThread::currentThread()->msleep(randomNumber(10, 25));

			// left click up
			mouseLeftClickUp(action.lastPosition);
		}

		// wait before next click
		QThread::currentThread()->msleep(randomNumber(30, action.delay));

		// stop auto-click if move the mouse
		if (QCursor::pos() != action.lastPosition)
		{
			m_stopClicker = 1;
			break;
		}

		emit changeSystrayIcon();

		// if not using simple mode
		if (!m_useSimpleMode)
		{
			endTime = QTime::currentTime();

			// check if we should pass to next spot
			if (startTime.msecsTo(endTime) > (action.duration * 1000))
			{
				// next spot
				++row;

				// new duration
				startTime = QTime::currentTime();

				// last spot, restart to first one
				if (row >= m_model->rowCount())
				{
					m_model->resetCount();
					row = 0;
				}

				// new spot
				action = m_model->getAction(row);

				// if next action is a repeat
				if (action.type == TypeRepeat)
				{
					// no more repeat
					if (action.lastCount < 1)
					{
						// next spot
						++row;

						// last spot, restart to first one
						if (row >= m_model->rowCount())
						{
							m_model->resetCount();
							row = 0;
						}

						// new spot
						action = m_model->getAction(row);
					}
					else
					{
						// decrease count
						--action.lastCount;

						qDebug() << "last count" << action.lastCount;

						m_model->setAction(row, action);

						// start
						row = 0;

						// repeat from start
						action = m_model->getAction(row);
					}
				}

				// apply window offset
				action.originalPosition += rect.topLeft();
				action.lastPosition = action.originalPosition;
			}
		}
	}

	if (m_stopClicker)
	{
		emit clickerStopped();
	}
}

void MainWindow::onNew()
{
	m_model->reset();

	updateStartButton();
}

void MainWindow::onOpen()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open script"), ConfigFile::getInstance()->getLocalDataDirectory(), "AutoClicker Files (*.acf)");

	if (filename.isEmpty()) return;

	if (m_model->load(filename))
	{
		QString filename = QFileInfo(m_model->getFilename()).baseName();

		m_ui->scriptLabel->setText(filename);

		updateStartButton();
	}
}

void MainWindow::onSave()
{
	m_model->save(m_model->getFilename());
}

void MainWindow::onSaveAs()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save actions"), /* ConfigFile::getInstance()->getLocalDataDirectory() */ m_model->getFilename(), "AutoClicker Files (*.acf)");

	if (filename.isEmpty()) return;

	m_model->save(filename);
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

	QSize size = ConfigFile::getInstance()->getTestDialogSize();
	if (!size.isNull()) s_dialog->resize(size);

	QPoint pos = ConfigFile::getInstance()->getTestDialogPosition();
	if (!pos.isNull()) s_dialog->move(pos);

	s_dialog->show();
}

void MainWindow::startListeningExternalInputEvents()
{
	// if cursor is outside window, begin to listen on keys
	if (!isHidden() && !rect().contains(mapFromGlobal(QCursor::pos())) && m_ui->startKeySequenceEdit->keySequence() != QKeySequence::UnknownKey)
	{
		// reset external listener
		m_stopExternalListener = 0;

		// start to listen for a key
		QtConcurrent::run(this, &MainWindow::listenExternalInputEvents);
	}
}

void MainWindow::listenExternalInputEvents()
{
	QKeySequence startKeySequence = m_ui->startKeySequenceEdit->keySequence();

	qint16 startKey = QKeySequenceToVK(startKeySequence);

	// no key defined
	if (startKey == 0) return;

	while (m_stopExternalListener == 0 && QThread::currentThread()->isRunning())
	{
		if (!underMouse())
		{
			bool buttonPressed = isKeyPressed(startKey);

			if (buttonPressed)
			{
				emit startSimple();

				// we need to stop autoclicking to relaunch thread
				return;
			}
		}

		QThread::currentThread()->msleep(50);
	}
}

void MainWindow::onStartKeyChanged(const QKeySequence &seq)
{
	ConfigFile::getInstance()->setStartKey(seq.toString());
}

void MainWindow::onPositionKeyChanged(const QKeySequence &seq)
{
	ConfigFile::getInstance()->setPositionKey(seq.toString());
}

void MainWindow::onDelayChanged(int delay)
{
	ConfigFile::getInstance()->setDelay(delay);
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
		tr("Tool to click automatically")+
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
		showNormal();
	}

	raise();
	activateWindow();
}

void MainWindow::onSystrayAction(SystrayIcon::SystrayAction action)
{
	switch(action)
	{
		case SystrayIcon::ActionUpdate:
		break;

		default:
		break;
	}
}

bool MainWindow::event(QEvent *e)
{
	if (e->type() == QEvent::WindowDeactivate)
	{
	}
	else if (e->type() == QEvent::Enter)
	{
		m_stopExternalListener = 1;
	}
	else if (e->type() == QEvent::Leave)
	{
		startListeningExternalInputEvents();
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
