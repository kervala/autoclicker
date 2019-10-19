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
#include "spot.h"

QDataStream& operator << (QDataStream& stream, const Spot &spot)
{
	stream << spot.name << spot.originalPosition << spot.delay;

	return stream;
}

QDataStream& operator >> (QDataStream& stream, Spot &spot)
{
	stream >> spot.name >> spot.originalPosition >> spot.delay;

	// copy original position
	spot.lastPosition = spot.originalPosition;

	return stream;
}
