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
#include "spot.h"

QDataStream& operator << (QDataStream& stream, const Action &action)
{
	stream << action.name << action.originalPosition << action.delayMin << action.delayMax << action.duration << action.type << action.originalCount;

	return stream;
}

QDataStream& operator >> (QDataStream& stream, Action& action)
{
	stream >> action.name >> action.originalPosition;
	
	if (stream.device()->property("version") >= 5)
	{
		stream >> action.name >> action.delayMin;
	}
	else
	{
		action.delayMin = 30;
	}

	stream >> action.delayMax;

	if (stream.device()->property("version") >= 2)
	{
		stream >> action.duration;

		if (stream.device()->property("version") >= 4)
		{
			stream >> action.type >> action.originalCount;
		}
		else
		{
			action.type = TypeClick;
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

QDataStream& operator << (QDataStream& stream, const ActionType& type)
{
	return stream << (int)type;
}

QDataStream& operator >> (QDataStream& stream, ActionType& type)
{
	int tmp;
	stream >> tmp;
	type = (ActionType)tmp;

	return stream;
}
