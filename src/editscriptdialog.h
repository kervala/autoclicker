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

#ifndef EDITSCRIPTDIALOG_H
#define EDITSCRIPTDIALOG_H

#include "action.h"

class ActionModel;
class QDataWidgetMapper;
class Updater;

namespace Ui
{
	class EditScriptDialog;
}

class EditScriptDialog : public QDialog
{
	Q_OBJECT

public:
	EditScriptDialog(QWidget* parent, ActionModel* model);
	virtual ~EditScriptDialog();

	ActionModel* getModel() const { return m_model; }

public slots:
	// buttons

	// spots
	void onInsertSpot();
	void onDeleteSpot();
	void onPosition();
	void onWindowTitleChanged();

	void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void onMousePositionChanged(const QPoint &pos);
	void onTypeChanged(int index);

	void setWindowTitleButton(const QString& title);

signals:
	void mousePositionChanged(const QPoint &pos);
	void windowTitleChanged(const QString& title);

protected:
	bool event(QEvent *e);

	void emitMousePosition();

	ActionModel *m_model;
	QDataWidgetMapper *m_mapper;

	QAtomicInt m_stopExternalListener;

	Ui::EditScriptDialog *m_ui;
};

#endif
