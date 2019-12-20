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

#ifndef SPOTMODEL_H
#define SPOTMODEL_H

#include "spot.h"

#include <QAbstractTableModel>

class SpotModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	SpotModel(QObject* parent = nullptr);
	virtual ~SpotModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
	bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;

	Qt::DropActions supportedDropActions() const override;
	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

	Spot getSpot(int row) const;
	void setSpot(int row, const Spot& spot);

	QString getWindowTitle() const;
	void setWindowTitle(const QString& name);

	void reset();

	bool load(const QString& filename);
	bool save(const QString& filename);

	bool updateSpotsPosition(const QPoint& offset);

	QString getFilename() const;

private:
	QList<Spot> m_spots;
	QString m_windowTitle;
	QString m_filename;
};

#endif
