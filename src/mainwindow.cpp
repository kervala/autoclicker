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
#include "capturedialog.h"

#ifdef Q_OS_WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinTaskbarButton>
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow() : QMainWindow(nullptr, Qt::WindowStaysOnTopHint), m_stopExternalListener(0), m_stopClicker(0), m_useSimpleMode(true)
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
	m_mapper->addMapping(m_ui->durationSpinBox, 3);

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
	connect(m_ui->startPushButton, &QPushButton::clicked, this, &MainWindow::onStartOrStop);
	connect(m_ui->positionPushButton, &QPushButton::clicked, this, &MainWindow::onPosition);
	connect(m_ui->windowNamePushButton, &QPushButton::clicked, this, &MainWindow::onWindowName);

	// Keys
	connect(m_ui->startKeySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &MainWindow::onStartKeyChanged);
	connect(m_ui->positionKeySequenceEdit, &QKeySequenceEdit::keySequenceChanged, this, &MainWindow::onPositionKeyChanged);

	QShortcut *shortcutDelete = new QShortcut(QKeySequence(Qt::Key_Delete), m_ui->spotsListView);
	connect(shortcutDelete, &QShortcut::activated, this, &MainWindow::onDeleteSpot);

	QShortcut* shortcutInsert = new QShortcut(QKeySequence(Qt::Key_Insert), m_ui->spotsListView);
	connect(shortcutInsert, &QShortcut::activated, this, &MainWindow::onInsertSpot);

#if FALSE && QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
	connect(m_ui->defaultDelaySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onDelayChanged);
#else
	connect(m_ui->defaultDelaySpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::onDelayChanged);
#endif

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
	connect(this, &MainWindow::windowName, this, &MainWindow::setWindowName);
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

void MainWindow::onInsertSpot()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

	int row = indices.isEmpty() ? -1:indices.front().row()+1;

	m_model->insertRow(row);
}

void MainWindow::onDeleteSpot()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

	if (indices.isEmpty()) return;

	int row = indices.front().row();

	m_model->removeRow(row);
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

void MainWindow::onStartOrStop()
{
	startOrStop(false);
}

void MainWindow::onStartSimple()
{
	// define unique spot parameters
	m_spot.delay = m_ui->defaultDelaySpinBox->value();
	m_spot.lastPosition = QCursor::pos();
	m_spot.originalPosition = m_spot.lastPosition;

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
	// wait a little
	if (!m_useSimpleMode) QThread::currentThread()->sleep(1);

	int row = 0;

	Spot spot;
	QTime startTime = QTime::currentTime();
	QTime endTime;

	if (m_useSimpleMode)
	{
		// simple mode
		spot = m_spot;
	}
	else
	{
		// multi mode
		spot = m_model->getSpot(row);
	}

	while(!m_stopClicker)
	{
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

			if (m_useSimpleMode)
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

		// left click down
		mouseLeftClickDown(spot.lastPosition);

		// between 6 and 14 clicks/second = 125-166

		// wait a little before releasing the mouse
		QThread::currentThread()->msleep(randomNumber(10, 25));

		// left click up
		mouseLeftClickUp(spot.lastPosition);

		// wait before next click
		QThread::currentThread()->msleep(randomNumber(30, spot.delay));

		// stop auto-click if move the mouse
		if (QCursor::pos() != spot.lastPosition)
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
			if (startTime.msecsTo(endTime) > (spot.duration * 1000))
			{
				// next spot
				++row;

				// new duration
				startTime = QTime::currentTime();

				// last spot, restart to first one
				if (row >= m_model->rowCount()) row = 0;

				// new spot
				spot = m_model->getSpot(row);
			}
		}
	}

	if (m_stopClicker)
	{
		emit clickerStopped();
	}
}

void MainWindow::emitMousePosition()
{
	QPoint pos = QCursor::pos();

	QWindow* window = QGuiApplication::topLevelAt(pos);

	if (window)
	{
		qDebug() << window->title();
	}
	else
	{
		qDebug() << "No window under cursor";
	}

	emit mousePosition(pos);
}

void MainWindow::onNew()
{
	m_model->reset();

	setWindowName("");
}

void MainWindow::onOpen()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open actions"), ConfigFile::getInstance()->getLocalDataDirectory(), "AutoClicker Files (*.acf)");

	if (filename.isEmpty()) return;

	if (m_model->load(filename))
	{
		setWindowName(m_model->getWindowName());
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

void MainWindow::onPosition()
{
	m_ui->positionPushButton->setEnabled(false);
	m_ui->positionPushButton->setText("???");

	m_waitingAction = ActionPosition;
}

void MainWindow::onWindowName()
{
	WId id = 0;
	QString name;

	{
		CaptureDialog dlg(this);

		if (!dlg.exec()) return;

		id = dlg.getWindowId();
		name = dlg.getWindowName();
	}

	// don't process window if minimized
	if (isWindowMinimized(id)) return;

	QRect rect = getWindowRect(id);

	m_model->setWindowName(name);
	m_model->updateSpotsPosition(rect.topLeft());

	// only update the push button label
	setWindowName(name);
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
	QKeySequence positionKeySequence = m_ui->positionKeySequenceEdit->keySequence();

	qint16 startKey = QKeySequenceToVK(startKeySequence);
	qint16 positionKey = QKeySequenceToVK(positionKeySequence);

	// no key defined
	if (startKey == 0 && positionKey == 0) return;

	while (m_stopExternalListener == 0 && QThread::currentThread()->isRunning())
	{
		if (!underMouse())
		{
			bool buttonPressed = isKeyPressed(startKey);

			if (buttonPressed)
			{
				emit startSimple();

				return;
			}

			buttonPressed = isKeyPressed(positionKey);

			if (buttonPressed)
			{
				emitMousePosition();

				return;
			}
		}

		QThread::currentThread()->msleep(10);
	}
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
	// only when position button is disabled
	if (m_ui->positionPushButton->isEnabled()) return;

	QModelIndex index = m_ui->spotsListView->selectionModel()->currentIndex();

	// update original and last positions
	Spot spot = m_model->getSpot(index.row());
	spot.lastPosition = pos;
	spot.originalPosition = pos;
	m_model->setSpot(index.row(), spot);

	m_ui->positionPushButton->setEnabled(true);
	m_ui->positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void MainWindow::setWindowName(const QString& name)
{
	QFontMetrics fm = m_ui->windowNamePushButton->fontMetrics();
	const int usableWidth = qRound(0.9 * m_ui->windowNamePushButton->width());

	QString elidedText = fm.elidedText(name, Qt::ElideRight, usableWidth);
	bool elided = (elidedText != name);

	QString text = elided ? elidedText : name;

	if (text.isEmpty()) text = tr("Unknown");

	m_ui->windowNamePushButton->setText(text);
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
		if (!isHidden())
		{
			if (!m_ui->positionPushButton->isEnabled())
			{
				// don't need to listen for a key anymore
				m_stopExternalListener = 1;

				emitMousePosition();
			}
		}
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
