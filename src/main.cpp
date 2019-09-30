/*
 *  AutoClicker is a tool to click automatically
 *  Copyright (C) 2017  Cedric OCHS
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <Windows.h>

// in ms
void randomSleep(int min, int max)
{
	int time = rand() * (max - min) / RAND_MAX + min;

	Sleep(time);
}

int main()
{
	srand((uint32_t)time(NULL));

	POINT pos, p;

	printf("Press <INSERT> to begin clicking, <ESC> to cancel and move your mouse to abord...\n");

	while (true)
	{
		// Check if <INSERT> is pressed
		if (GetAsyncKeyState(VK_INSERT))
		{
			// grab position
			GetCursorPos(&pos);

			break;
		}
		else if (GetAsyncKeyState(VK_ESCAPE))
		{
			return 0;
		}

		// wait 100ms
		Sleep(100);
	}

	while (true)
	{
		// abort if move the mouse or click on Escape
		if ((GetCursorPos(&p) && p.x != pos.x && p.y != pos.y)) break;

		mouse_event(MOUSEEVENTF_LEFTDOWN, pos.x, pos.y, 0, 0);

		// between 6 and 8 clicks/second = 125-166

		// wait a little before releasing the mouse
		randomSleep(10, 25);
		mouse_event(MOUSEEVENTF_LEFTUP, pos.x, pos.y, 0, 0);

		// wait before reclicking
		randomSleep(50, 150);
	}

	return 0;
}
