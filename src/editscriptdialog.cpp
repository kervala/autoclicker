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

#include "common.h"
#include "editscriptdialog.h"
//#include "moc_editscriptdialog.cpp"
#include "ui_editscriptdialog.h"
#include "configfile.h"
#include "actionmodel.h"
#include "utils.h"
#include "capturedialog.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

EditScriptDialog::EditScriptDialog(QWidget *parent, ActionModel *model):QDialog(parent), m_stopExternalListener(0)
{
	m_ui = new Ui::EditScriptDialog();
	m_ui->setupUi(this);

	// clone the model to not change it if cancel dialog
	m_model = model->clone(this);

	m_ui->spotsListView->setModel(m_model);

	m_ui->actionGroupBox->setVisible(false);

	m_mapper = new QDataWidgetMapper(this);
	m_mapper->setModel(m_model);
	m_mapper->addMapping(m_ui->nameLineEdit, ActionColumnName);
	m_mapper->addMapping(m_ui->typeComboBox, ActionColumnType, "currentIndex");
	m_mapper->addMapping(m_ui->delayMinSpinBox, ActionColumnDelayMin);
	m_mapper->addMapping(m_ui->delayMaxSpinBox, ActionColumnDelayMax);
	m_mapper->addMapping(m_ui->durationSpinBox, ActionColumnDuration);
	m_mapper->addMapping(m_ui->countSpinBox, ActionColumnOriginalCount);

	QStandardItemModel* typesModel = new QStandardItemModel(this);
	typesModel->appendRow(new QStandardItem(tr("None")));
	typesModel->appendRow(new QStandardItem(tr("Click")));
	typesModel->appendRow(new QStandardItem(tr("Repeat")));

	m_ui->typeComboBox->setModel(typesModel);

	connect(m_ui->typeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &EditScriptDialog::onTypeChanged);

	// Buttons
	connect(m_ui->positionPushButton, &QPushButton::clicked, this, &EditScriptDialog::onPosition);
	connect(m_ui->windowTitlePushButton, &QPushButton::clicked, this, &EditScriptDialog::onWindowTitleChanged);

	QShortcut *shortcutDelete = new QShortcut(QKeySequence(Qt::Key_Delete), m_ui->spotsListView);
	connect(shortcutDelete, &QShortcut::activated, this, &EditScriptDialog::onDeleteSpot);

	QShortcut* shortcutInsert = new QShortcut(QKeySequence(Qt::Key_Insert), m_ui->spotsListView);
	connect(shortcutInsert, &QShortcut::activated, this, &EditScriptDialog::onInsertSpot);

	// Selection model
	connect(m_ui->spotsListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EditScriptDialog::onSelectionChanged);
	connect(m_ui->spotsListView->selectionModel(), &QItemSelectionModel::currentRowChanged, m_mapper, &QDataWidgetMapper::setCurrentModelIndex);

	// MainWindow
	connect(this, &EditScriptDialog::mousePositionChanged, this, &EditScriptDialog::onMousePositionChanged);
	connect(this, &EditScriptDialog::windowTitleChanged, this, &EditScriptDialog::setWindowTitleButton);

	setWindowTitleButton(m_model->getWindowTitle());
}

EditScriptDialog::~EditScriptDialog()
{
	delete m_ui;
}

void EditScriptDialog::onInsertSpot()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

	int row = indices.isEmpty() ? -1:indices.front().row()+1;

	m_model->insertRow(row);
}

void EditScriptDialog::onDeleteSpot()
{
	QModelIndexList indices = m_ui->spotsListView->selectionModel()->selectedRows();

	if (indices.isEmpty()) return;

	int row = indices.front().row();

	m_model->removeRow(row);
}

void EditScriptDialog::emitMousePosition()
{
	// absolute coordinates
	QPoint pos = QCursor::pos();

	QString title = m_model->getWindowTitle();

	if (!title.isEmpty())
	{
		// search a window with title
		Window window = getWindowWithTitle(title);

		// if found
		if (window.id)
		{
			// convert to relative coordinates
			pos -= window.rect.topLeft();
		}
		else
		{
			qDebug() << "No window with title" << title;
		}
	}

	emit mousePositionChanged(pos);
}

void EditScriptDialog::onPosition()
{
	m_ui->positionPushButton->setEnabled(false);
	m_ui->positionPushButton->setText("???");

	// if cursor is outside window, begin to listen on keys
	if (!rect().contains(mapFromGlobal(QCursor::pos())) && !ConfigFile::getInstance()->getPositionKey().isEmpty())
	{
		// reset external listener
		m_stopExternalListener = 0;

		// start to listen for a key
		QtConcurrent::run(&EditScriptDialog::listenExternalInputEvents, this);
	}
}

void EditScriptDialog::onWindowTitleChanged()
{
	Window window;

	{
		CaptureDialog dlg(this);

		if (!dlg.exec()) return;

		window = dlg.getWindow();
	}

	// don't process window if minimized
	if (isWindowMinimized(window.id)) return;

	m_model->setWindowTitle(window.title);
	m_model->updateSpotsPosition(window.rect.topLeft());

	// only update the push button label
	setWindowTitleButton(window.title);
}

void EditScriptDialog::listenExternalInputEvents()
{
	QKeySequence positionKeySequence = ConfigFile::getInstance()->getPositionKey();

	qint16 positionKey = QKeySequenceToVK(positionKeySequence);

	// no key defined
	if (positionKey == 0) return;

	while (m_stopExternalListener == 0 && QThread::currentThread()->isRunning())
	{
		if (!underMouse() && isKeyPressed(positionKey))
		{
			emitMousePosition();
		}

		QThread::currentThread()->msleep(50);
	}
}

void EditScriptDialog::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	// update controls
	m_ui->actionGroupBox->setVisible(!selected.empty());

	if (selected.empty()) return;

	// update controls
	const QItemSelectionRange &range = selected.front();

	QModelIndexList indices = range.indexes();

	int row = indices.front().row();

	Action action = m_model->getAction(row);

	// manually update position because not handled by data mapper
	QPoint pos = action.originalPosition;

	m_ui->positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));

	onTypeChanged(typeToInt(action.type));
}

void EditScriptDialog::onMousePositionChanged(const QPoint& pos)
{
	// only when position button is disabled
	if (m_ui->positionPushButton->isEnabled()) return;

	QModelIndex index = m_ui->spotsListView->selectionModel()->currentIndex();

	// update original and last positions
	Action spot = m_model->getAction(index.row());
	spot.lastPosition = pos;
	spot.originalPosition = pos;
	m_model->setAction(index.row(), spot);

	m_ui->positionPushButton->setEnabled(true);
	m_ui->positionPushButton->setText(QString("(%1, %2)").arg(pos.x()).arg(pos.y()));
}

void EditScriptDialog::onTypeChanged(int index)
{
	switch (typeFromInt(index))
	{
	case Action::Type::Repeat:
		m_ui->durationLabel->setVisible(false);
		m_ui->durationSpinBox->setVisible(false);

		m_ui->positionLabel->setVisible(false);
		m_ui->positionPushButton->setVisible(false);

		m_ui->countLabel->setVisible(true);
		m_ui->countSpinBox->setVisible(true);

		break;

	default:
		m_ui->durationLabel->setVisible(true);
		m_ui->durationSpinBox->setVisible(true);

		m_ui->positionLabel->setVisible(true);
		m_ui->positionPushButton->setVisible(true);

		m_ui->countLabel->setVisible(false);
		m_ui->countSpinBox->setVisible(false);
	}
}

void EditScriptDialog::setWindowTitleButton(const QString& title)
{
	QFontMetrics fm = m_ui->windowTitlePushButton->fontMetrics();
	const int usableWidth = qRound(0.9 * m_ui->windowTitlePushButton->width());

	QString elidedText = fm.elidedText(title, Qt::ElideRight, usableWidth);
	bool elided = (elidedText != title);

	QString text = elided ? elidedText : title;

	if (text.isEmpty()) text = tr("Unknown");

	m_ui->windowTitlePushButton->setText(text);
}

bool EditScriptDialog::event(QEvent *e)
{
	if (e->type() == QEvent::WindowDeactivate)
	{
		emitMousePosition();
	}
	else if (e->type() == QEvent::Enter)
	{
		m_stopExternalListener = 1;
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

	return QDialog::event(e);
}
