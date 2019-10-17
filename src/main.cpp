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
#include <vector>

int random(int max)
{
	return (int)((float)(rand() * (max + 1)) / (float)(RAND_MAX + 1));
}

// in ms
void randomSleep(int min, int max)
{
	int time = random(max - min) + min;

	Sleep(time);
}

int randomPos(int org, int max)
{
	return org + (rand() * (2 * max) / RAND_MAX) - max;
}

struct Spot
{
	// original position of the user click
	POINT originalPos;

	// random position
	POINT lastPos;

	// number of clicks before to switch to next spot
	int clicks;
};

bool checkKeys(std::vector<Spot> &spots, bool &click, int &speed)
{
	// Check if <INSERT> is pressed
	if (GetAsyncKeyState(VK_INSERT))
	{
		Spot spot;
		spot.clicks = 50;

		// grab position
		GetCursorPos(&spot.originalPos);

		printf("Origin position of cursor is (%d, %d)\n", spot.originalPos.x, spot.originalPos.y);

		spot.lastPos = spot.originalPos;

		// append this spot
		spots.push_back(spot);
	}
	if (GetAsyncKeyState(VK_HOME))
	{
		click = true;
	}
	else if (GetAsyncKeyState(VK_ADD))
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
	else if (GetAsyncKeyState(VK_DIVIDE))
	{
		if (spots.empty())
		{
			printf("No previous spot defined\n");
		}
		else
		{
			// decrease clicks
			spots.back().clicks /= 2;

			if (spots.back().clicks == 0) spots.back().clicks = 1;

			printf("Decrease number of clicks to %d\n", spots.back().clicks);
		}
	}
	else if (GetAsyncKeyState(VK_MULTIPLY))
	{
		if (spots.empty())
		{
			printf("No previous spot defined\n");
		}
		else
		{
			// increase clicks
			spots.back().clicks *= 2;

			printf("Increase number of clicks to %d\n", spots.back().clicks);
		}
	}
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		return false;
	}

	if (speed < 40)
	{
		printf("Speed %d ms faster than maximum (40 ms), cap it...\n", speed);

		speed = 40;
	}

	return true;
}

int main()
{
	srand((uint32_t)time(NULL));

	// checked position
	POINT p;

	int speed = 150;

	bool click = false;

	int dx = 0;
	int dy = 0;

	printf("Press :\n");
	printf("- <INSERT> to add a position\n");
	printf("- <HOME> to begin clicking\n");
	printf("- </> or <*> to decrease/increase number of clicks\n");
	printf("- <+> or <-> to increase/decrease speed (default is up to 150 ms)\n");
	printf("- <ESC> to cancel\n");
	printf("- Move your mouse to abord...\n");

	std::vector<Spot> spots;

	while (true)
	{
		if (click && !spots.empty())
		{
			// process each spot
			for (size_t i = 0; i < spots.size(); ++i)
			{
				Spot& spot = spots[i];

				// generate 50 clicks before change spot
				for (int j = 0; j < spot.clicks; ++j)
				{
					// check key presses
					if (!checkKeys(spots, click, speed)) return 0;

					// set cursor position
					SetCursorPos(spot.lastPos.x, spot.lastPos.y);

					// left click down
					mouse_event(MOUSEEVENTF_LEFTDOWN, spot.lastPos.x, spot.lastPos.y, 0, 0);

					// between 6 and 14 clicks/second = 125-166

					// wait a little before releasing the mouse
					randomSleep(10, 25);

					// left click up
					mouse_event(MOUSEEVENTF_LEFTUP, spot.lastPos.x, spot.lastPos.y, 0, 0);

					printf("Click at (%d, %d)\n", spot.lastPos.x, spot.lastPos.y);

					// wait before reclicking
					randomSleep(30, speed);

					// stop auto-click if move the mouse
					if (GetCursorPos(&p) && p.x != spot.lastPos.x && p.y != spot.lastPos.y)
					{
						click = false;
					}

					// 50% change position
					if (random(1) == 0)
					{
						// randomize position
						dx = random(2) - 1;
						dy = random(2) - 1;

						// invert sign
						if ((spot.lastPos.x + dx > (spot.originalPos.x + 5)) || (spot.lastPos.x + dx < (spot.originalPos.x - 5))) dx = -dx;
						if ((spot.lastPos.y + dy > (spot.originalPos.y + 5)) || (spot.lastPos.y + dx < (spot.originalPos.y - 5))) dy = -dy;

						spot.lastPos.x += dx;
						spot.lastPos.y += dy;
					}
				}
			}
		}
		else
		{
			// check key presses
			if (!checkKeys(spots, click, speed)) return 0;

			// wait 100ms
			Sleep(100);
		}
	}

	return 0;
}
