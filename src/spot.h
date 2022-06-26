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

#ifndef ACTION_H
#define ACTION_H

struct Action
{
	enum class Type
	{
		None,
		Click,
		Repeat
	};

	Action() :type(Type::None), delayMin(0), delayMax(0), duration(0), originalCount(0), lastCount(0)
	{
	}

	QString name;
	Type type;
	QPoint originalPosition;
	int delayMin;
	int delayMax;
	QPoint lastPosition;
	int duration;
	int originalCount;
	int lastCount;
};

QString typeToString(Action::Type type);
Action::Type typeFromString(const QString &type);

int typeToInt(Action::Type type);
Action::Type typeFromInt(int type);

QDataStream& operator << (QDataStream& stream, const Action& action);
QDataStream& operator >> (QDataStream& stream, Action& action);

QDataStream& operator << (QDataStream& stream, const Action::Type& type);
QDataStream& operator >> (QDataStream& stream, Action::Type& type);

#endif
