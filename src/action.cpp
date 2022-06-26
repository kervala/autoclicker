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
#include "spot.h"

QString pointToString(const QPoint& point)
{
	return QString("(%1,%2)").arg(point.x()).arg(point.y());
}

QString typeToString(Action::Type type)
{
	switch (type)
	{
	case Action::Type::Click: return "click";
	case Action::Type::Repeat: return "repeat";
	default: break;
	}

	return "none";
}

Action::Type typeFromString(const QString& type)
{
	if (type == "click")
	{
		return Action::Type::Click;
	}

	if (type == "repeat")
	{
		return Action::Type::Repeat;
	}

	return Action::Type::None;
}

int typeToInt(Action::Type type)
{
	switch (type)
	{
	case Action::Type::Click: return 1;
	case Action::Type::Repeat: return 2;
	default: break;
	}

	return 0;
}

Action::Type typeFromInt(int type)
{
	switch (type)
	{
	case 1: return Action::Type::Click;
	case 2: return Action::Type::Repeat;
	default: break;
	}

	return Action::Type::None;
}

QString Action::toString() const
{
	return QString("\"%1\" %2 %3-%4 %5 %6 %7").arg(name)
		.arg(pointToString(originalPosition))
		.arg(delayMin)
		.arg(delayMax)
		.arg(duration)
		.arg(typeToString(type))
		.arg(originalCount);
}

Action Action::fromString(const QString& str)
{
	QRegularExpression regex("\"(.*)\" \\(([0-9]+)\\,([0-9]+)\\) ([0-9]+)-([0-9]+) ([a-z]+) ([0-9]+)");

	QRegularExpressionMatch match = regex.match(str);

	Action action;

	if (match.hasMatch())
	{
		action.name = match.captured(1);
		action.originalPosition.setX(match.captured(2).toInt());
		action.originalPosition.setY(match.captured(3).toInt());
		action.delayMin = match.captured(4).toInt();
		action.delayMax = match.captured(5).toInt();
		action.duration = match.captured(6).toInt();
		action.type = typeFromString(match.captured(7));
		action.originalCount = match.captured(8).toInt();
	}

	return action;
}

bool Action::readFromSettings(QSettings& settings)
{
	name = settings.value("Name").toString();
	type = typeFromString(settings.value("Type").toString());
	originalPosition = settings.value("OriginalPosition").toPoint();
	originalCount = settings.value("OriginalCount").toInt();
	delayMin = settings.value("DelayMin").toInt();
	delayMax = settings.value("DelayMax").toInt();
	duration = settings.value("Duration").toInt();

	return true;
}

bool Action::writeToSettings(QSettings& settings) const
{
	settings.setValue("Name", name);
	settings.setValue("Type", typeToString(type));
	settings.setValue("OriginalPosition", originalPosition);
	settings.setValue("OriginalCount", originalCount);
	settings.setValue("DelayMin", delayMin);
	settings.setValue("DelayMax", delayMax);
	settings.setValue("Duration", duration);

	return true;
}

QDataStream& operator << (QDataStream& stream, const Action &action)
{
	stream << action.name << action.originalPosition << action.delayMin << action.delayMax << action.duration << action.type << action.originalCount;

	return stream;
}

QDataStream& operator >> (QDataStream& stream, Action& action)
{
	stream >> action.name >> action.originalPosition;
	
	if (stream.device()->property("version").toInt() >= 5)
	{
		stream >> action.delayMin;
	}
	else
	{
		action.delayMin = 30;
	}

	stream >> action.delayMax;

	if (stream.device()->property("version").toInt() >= 2)
	{
		stream >> action.duration;

		if (stream.device()->property("version").toInt() >= 4)
		{
			stream >> action.type >> action.originalCount;
		}
		else
		{
			action.type = Action::Type::Click;
			action.originalCount = 0;
		}
	}
	else
	{
		action.duration = 0;
	}

	// copy original position
	action.lastPosition = action.originalPosition;

	// copy original count
	action.lastCount = action.originalCount;

	return stream;
}

QDataStream& operator << (QDataStream& stream, const Action::Type& type)
{
	return stream << typeToInt(type);
}

QDataStream& operator >> (QDataStream& stream, Action::Type& type)
{
	int tmp;
	stream >> tmp;
	type = typeFromInt(tmp);

	return stream;
}
