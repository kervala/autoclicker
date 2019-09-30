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

int randomPos(int org, int max)
{
	return org + (rand() * (2 * max) / RAND_MAX) - max;
}

int main()
{
	srand((uint32_t)time(NULL));

	POINT pos, p, pos2;

	int speed = 150;

	printf("Press <INSERT> to begin clicking, <+> or <-> to increase/decrease speed (default is up to 150 ms), <ESC> to cancel and move your mouse to abord...\n");

	while (true)
	{
		// Check if <INSERT> is pressed
		if (GetAsyncKeyState(VK_INSERT))
		{
			// grab position
			GetCursorPos(&pos);

			printf("Origin position of cursor is (%d, %d)\n", pos.x, pos.y);

			pos2 = pos;

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
		if (GetAsyncKeyState(VK_ADD))
		{
			// increase speed
			speed -= 10;

			printf("Increase speed to %d ms\n", speed);
		}
		else if (GetAsyncKeyState(VK_SUBTRACT))
		{
			// decrease speed
			speed += 10;

			printf("Decrease speed to %d ms\n", speed);
		}
		else if (GetAsyncKeyState(VK_ESCAPE))
		{
			return 0;
		}

		if (speed < 40)
		{
			printf("Speed %d ms faster than maximum (40 ms), cap it...\n", speed);

			speed = 40;
		}

		mouse_event(MOUSEEVENTF_LEFTDOWN, pos2.x, pos2.y, 0, 0);

		// between 6 and 14 clicks/second = 125-166

		// wait a little before releasing the mouse
		randomSleep(10, 25);
		mouse_event(MOUSEEVENTF_LEFTUP, pos2.x, pos2.y, 0, 0);

		// wait before reclicking
		randomSleep(30, speed);

		// abort if move the mouse or click on Escape
		if (GetCursorPos(&p))
		{
			if (p.x != pos2.x && p.y != pos2.y) break;

			printf("Cursor at at (%d, %d)\n", p.x, p.y);
		}

		// randomize position
		pos2.x = randomPos(pos.x, 5);
		pos2.y = randomPos(pos.y, 5);

		// set cursor position
		SetCursorPos(pos2.x, pos2.y);

		printf("Click at (%d, %d)\n", pos2.x, pos2.y);
	}

	return 0;
}
