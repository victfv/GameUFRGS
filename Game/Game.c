#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAP_HEIGHT 11
#define MAP_WIDTH 27
#define LOOP_DELAY 0.0333332
#define SWORD_INC 8
#define AI_MOVE_DELAY 1

//Geral

int win = 0;

typedef struct
{
	int x;
	int y;
} vec2;

typedef struct
{
	int alive;
	vec2 position;
	int dir;
} ogre;

int running = 0;
int paused = 0;

//Timing
float elapsedTimeS;

//Player
vec2 playerPosition;
vec2 nextPosition;
vec2 playerSpawn;
int playerLives;
int playerCoins;
int playerKeys;
int lastCoins = 9;
float swordTime;

//AI
ogre ogres[4];
int totalOgres;
int ogresAlive;
int lastMovTime = 0;

//Map
int mapN = 1;
char map[MAP_HEIGHT][MAP_WIDTH];
vec2 *dlCoins;
vec2 *dlKeys;
int totalKeys;
int totalCoins;

//Menus
int selectedOption = 0;
int currentMenu = 0;
int maxOption = 2;







//Geral////////////////////////////////////////////////////////////////////////////////////////////////////////////
void delay(float secs)//Função de delay (utilizada para travar framerate)
{
	int millis = (int)(1000 * secs);
	clock_t startTime = clock();
	while (clock() < startTime + millis);
}

void timer()
{
	elapsedTimeS += LOOP_DELAY;
}


//Carregar/Salvar////////////////////////////////////////////////////////////////////////////////////////////////////////////
int loadMap(int mapId, int resetStats)
{
	int i, j, r;
	int rnd = rand() % 4;
	char fileName[10], symbol;
	FILE *mapFile;

	sprintf_s(fileName, sizeof(char) * 10, "Map%02d.txt", mapId);

	if (fopen_s(&mapFile, fileName, "r") == 0)
	{
		totalOgres = 0;
		totalCoins = 0;
		totalKeys = 0;
		lastMovTime = 0;
		free(dlCoins);
		free(dlKeys);

		if (resetStats)
		{
			playerCoins = 0;
			playerKeys = 0;
			playerLives = 3;
			swordTime = 0;
			lastCoins = 9;
			elapsedTimeS = 0;
		}

		for (i = 0; i < MAP_HEIGHT; i++)
		{
			for (j = 0; j <= MAP_WIDTH; j++)
			{
				symbol = fgetc(mapFile);
				if (symbol != '\n')
				{
					symbol = tolower(symbol);
					map[i][j] = symbol;
				}
			}
		}

		for (i = 0; i < MAP_HEIGHT; i++)
		{
			for (j = 0; j <= MAP_WIDTH; j++)
			{
				symbol = map[i][j];
				switch (symbol)
				{
				case 'o':
					map[i][j] = ' ';
					ogres[totalOgres].alive = 1;
					ogres[totalOgres].position.x = j;
					ogres[totalOgres].position.y = i;
					ogres[totalOgres].dir = rnd;
					totalOgres++;
					break;
				case 'j':
					map[i][j] = ' ';
					playerSpawn.x = j;
					playerSpawn.y = i;
					playerPosition.x = j;
					playerPosition.y = i;
					break;
				case 'm':
					totalCoins++;
					break;
				case 'c':
					totalKeys++;
					break;
				default:
					break;
				}
			}
		}

		dlCoins = malloc(sizeof(vec2) * totalCoins);
		dlKeys = malloc(sizeof(vec2) * totalKeys);

		ogresAlive = totalOgres;

		if (fclose(mapFile) == 0)
		{
			r = 0;
		}
		else
		{
			r = 1;
		}
	}
	else
	{
		r = 1;
	}
	return r;
}

int saveGame(int slot)
{
	FILE *saveFile;
	char saveName[6];
	int i, r;

	sprintf_s(saveName, 6, "%d.sav", slot);

	if (fopen_s(&saveFile, saveName, "w") == 0)
	{
		fprintf_s(saveFile, "s%d\n", mapN);
		fprintf_s(saveFile, "c%d,%d\n", playerPosition.x, playerPosition.y);
		fprintf_s(saveFile, "s%d\n", playerCoins);
		fprintf_s(saveFile, "s%d\n", playerLives);
		fprintf_s(saveFile, "s%d\n", playerKeys);
		fprintf_s(saveFile, "s%d\n", (int)elapsedTimeS);
		fprintf_s(saveFile, "s%d\n", (int)swordTime);
		fprintf_s(saveFile, "s%d\n", lastCoins);
		fprintf_s(saveFile, "s%d\n", totalOgres);
		fprintf_s(saveFile, "s%d\n", ogresAlive);
		for (i = 0; i < totalOgres; i++)
		{
			if (ogres[i].alive)
			{
				fprintf_s(saveFile, "c%d,%d\n", ogres[i].position.x, ogres[i].position.y);
			}
		}
		for (i = 0; i < playerCoins; i++)
		{
			fprintf_s(saveFile, "c%d,%d\n", dlCoins[i].x, dlCoins[i].y);
		}
		for (i = 0; i < playerKeys; i++)
		{
			fprintf_s(saveFile, "c%d,%d\n", dlKeys[i].x, dlKeys[i].y);
		}
		if (fclose(saveFile) == 0)
		{
			r = 0;
		}
		else
		{
			r = 1;
		}
	}
	else
	{
		r = 1;
	}
	return r;
}

int extractLine(FILE *save, int *num, int *x, int *y)
{
	int i, s, type;
	char buff[3] = {"aa"};
	s = fgetc(save);
	switch (s)
	{
	case 's':
		type = 0;
		for (i = 0; i < 3; i++)
		{
			s = fgetc(save);
			if (s != '\n')
			{
				buff[i] = s;
			}
			else
			{
				break;
			}
		}
		*num = atoi(buff);
		*x = -1;
		*y = -1;
		break;
	case 'c':
		type = 1;
		for (i = 0; i < 3; i++)
		{
			s = fgetc(save);
			if (s != ',')
			{
				buff[i] = s;
			}
			else
			{
				break;
			}
		}
		*x = atoi(buff);
		sprintf_s(buff, 3, "aa");
		for (i = 0; i < 3; i++)
		{
			s = fgetc(save);
			if (s != '\n')
			{
				buff[i] = s;
			}
			else
			{
				break;
			}
		}
		*y = atoi(buff);
		*num = -1;
		break;
	default:
		exit(54);
	}
	return type;
}

int loadGame(int slot)
{
	FILE *saveFile;
	char saveName[6];
	int i, r = 1;
	int num, x, y;
	
	sprintf_s(saveName, 6, "%d.sav", slot);

	if (fopen_s(&saveFile, saveName, "r") == 0)
	{
		extractLine(saveFile, &num, &x, &y);
		mapN = num;
		loadMap(mapN, 0);
		extractLine(saveFile, &num, &x, &y);
		playerPosition.x = x;
		playerPosition.y = y;
		nextPosition.x = x;
		nextPosition.y = y;
		extractLine(saveFile, &num, &x, &y);
		playerCoins = num;
		extractLine(saveFile, &num, &x, &y);
		playerLives = num;
		extractLine(saveFile, &num, &x, &y);
		playerKeys = num;
		extractLine(saveFile, &num, &x, &y);
		elapsedTimeS = (float)num;
		extractLine(saveFile, &num, &x, &y);
		swordTime = (float)num;
		extractLine(saveFile, &num, &x, &y);
		lastCoins = num;
		extractLine(saveFile, &num, &x, &y);
		totalOgres = num;
		extractLine(saveFile, &num, &x, &y);
		ogresAlive = num;
		for (i = 0; i < totalOgres; i++)
		{
			if (i < ogresAlive)
			{
				extractLine(saveFile, &num, &x, &y);
				ogres[i].alive = 1;
				ogres[i].position.x = x;
				ogres[i].position.y = y;
			}
			else
			{
				ogres[i].alive = 0;
			}
		}

		for (i = 0; i < playerCoins; i++)
		{
			extractLine(saveFile, &num, &x, &y);
			dlCoins[i].x = x;
			dlCoins[i].y = y;
			map[y][x] = ' ';
		}

		for (i = 0; i < playerKeys; i++)
		{
			extractLine(saveFile, &num, &x, &y);
			dlKeys[i].x = x;
			dlKeys[i].y = y;
			map[y][x] = ' ';
		}
		fclose(saveFile);
		r = 0;
	}
	else
	{
		r = 1;
	}
	return r;
}

//Menus////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initPairs()
{
	init_pair(1, COLOR_BLACK, COLOR_RED);//Wall
	init_pair(2, COLOR_YELLOW, COLOR_WHITE);//Coins/Keys
	init_pair(3, COLOR_RED, COLOR_WHITE);//Player
	init_pair(8, COLOR_RED, COLOR_YELLOW);//InvinciblePlayer
	init_pair(4, COLOR_RED, COLOR_MAGENTA);//Closed door
	init_pair(5, COLOR_GREEN, COLOR_WHITE);//Enemy
	init_pair(6, COLOR_YELLOW, COLOR_GREEN);//Switch
	init_pair(7, COLOR_BLACK, COLOR_CYAN);//Selected option
	init_pair(9, COLOR_BLACK, COLOR_WHITE);//Open door
	init_pair(10, COLOR_YELLOW, COLOR_BLACK);//Win text
	init_pair(11, COLOR_CYAN, COLOR_BLACK);
}

void activateMenuHighlight(int option)
{
	if (option == selectedOption)
	{
		attron(COLOR_PAIR(7));
	}
}

void deactivateMenuHighlight()
{
	attroff(COLOR_PAIR(7));
}


void mainMenuR()//ID 0
{
	if (running == 0)
	{
		activateMenuHighlight(0);
		mvprintw(1, 2, "New Game");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(3, 2, "Load Game");
		deactivateMenuHighlight();

		activateMenuHighlight(2);
		mvprintw(5, 2, "Quit");
		deactivateMenuHighlight();
	}
	else
	{
		activateMenuHighlight(0);
		mvprintw(1, 2, "Resume");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(3, 2, "Save Game");
		deactivateMenuHighlight();

		activateMenuHighlight(2);
		mvprintw(5, 2, "Quit");
		deactivateMenuHighlight();
	}
}

void loadSaveMenuR()//Id 1
{
	if (running)
	{
		activateMenuHighlight(0);
		mvprintw(1, 2, "Save Slot 1");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(3, 2, "Save Slot 2");
		deactivateMenuHighlight();

		activateMenuHighlight(2);
		mvprintw(5, 2, "Save Slot 3");
		deactivateMenuHighlight();

		activateMenuHighlight(3);
		mvprintw(7, 2, "Cancel");
		deactivateMenuHighlight();
	}
	else
	{
		activateMenuHighlight(0);
		mvprintw(1, 2, "Load Slot 1");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(3, 2, "Load Slot 2");
		deactivateMenuHighlight();

		activateMenuHighlight(2);
		mvprintw(5, 2, "Load Slot 3");
		deactivateMenuHighlight();

		activateMenuHighlight(3);
		mvprintw(7, 2, "Cancel");
		deactivateMenuHighlight();
	}
}

void endingR()
{
	mvprintw(1, 2, "The End");

	activateMenuHighlight(0);
	mvprintw(3, 2, "Quit");
	deactivateMenuHighlight();
}


void endGameR()//Id 2
{
	if (win == 1)
	{
		mvprintw(1, 2, "You Win!");

		mvprintw(2, 2, "Score: %d", (totalOgres - ogresAlive)*100 + playerCoins * 10);

		activateMenuHighlight(0);
		mvprintw(4, 2, "Continue");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(6, 2, "Quit");
		deactivateMenuHighlight();
	}
	else
	{
		mvprintw(1, 2, "You Lose!");

		activateMenuHighlight(0);
		mvprintw(3, 2, "Restart");
		deactivateMenuHighlight();

		activateMenuHighlight(1);
		mvprintw(5, 2, "Quit");
		deactivateMenuHighlight();
	}
}

void renderMenus()
{
	clear();
	switch (currentMenu)
	{
	case 0:
		mainMenuR();
		break;
	case 1:
		loadSaveMenuR();
		break;
	case 2:
		endGameR();
		break;
	case 3:
		endingR();
		break;
	}
	refresh();
}

interactWithMenu()
{
	switch (currentMenu)
	{
	case 0:
		switch (selectedOption)
		{
		case 0:
			if (running == 0)
			{
				loadMap(1, 1);
				running = 1;
			}
			else
			{
				paused = 0;
			}
			break;
		case 1:
			currentMenu = 1;
			break;
		case 2:
			exit(0);
			break;
		}
		break;
	case 1:
		if (running == 0)
		{
			switch (selectedOption)
			{
			case 0:
				if (loadGame(1) != 0)
				{
					mvprintw(1, 2, "Could not load save.");
					refresh();
					delay(1);
				}
				else
				{
					running = 1;
					currentMenu = 0;
				}
				break;
			case 1:
				if (loadGame(2) != 0)
				{
					mvprintw(1, 2, "Could not load save.");
					refresh();
					delay(1);
				}
				else
				{
					running = 1;
					currentMenu = 0;
				}
				break;
			case 2:
				if (loadGame(3) != 0)
				{
					mvprintw(1, 2, "Could not load save.");
					refresh();
					delay(1);
				}
				else
				{
					running = 1;
					currentMenu = 0;
				}
				break;
			case 3:
				currentMenu = 0;
				break;
			}
		}
		else
		{
			switch (selectedOption)
			{
			case 0:
				if (saveGame(1) != 0)
				{
					mvprintw(1, 2, "Could not save game.");
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");
					refresh();
					delay(1);
					currentMenu = 0;
				}
				break;
			case 1:
				if (saveGame(2) != 0)
				{
					mvprintw(1, 2, "Could not save game.");
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");
					refresh();
					delay(1);
					currentMenu = 0;
				}
				break;
			case 2:
				if (saveGame(3) != 0)
				{
					mvprintw(1, 2, "Could not save game.");
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");
					refresh();
					delay(1);
					currentMenu = 0;
				}
				break;
			case 3:
				currentMenu = 0;
				break;
			}
		}
		break;
		case 2:
			switch (selectedOption)
			{
			case 0:
				if (win)
				{
					mapN++;
					if (loadMap(mapN, 1) == 0)
					{
						paused = 0;
					}
					else
					{
						currentMenu = 3;
					}
				}
				else
				{
					loadMap(mapN, 1);
					paused = 0;
				}
				break;
			case 1:
				exit(0);
			}
			break;
	case 3:
		exit(0);
		break;
	}
	selectedOption = 0;
}

void menuInput()
{
	int key = getch();

	switch (key)
	{
	case KEY_DOWN:
	case 's':
	case 'S':
		if (selectedOption < maxOption)
		{
			selectedOption++;
		}
		break;
	case KEY_UP:
	case 'w':
	case 'W':
		if (selectedOption > 0)
		{
			selectedOption--;
		}
		break;
	case '\n':
		interactWithMenu();
		break;
	}
}

void menuManager()
{
	switch (currentMenu)
	{
	case 0:
		maxOption = 2;
		break;
	case 1:
		maxOption = 3;
		break;
	case 2:
		maxOption = 1;
		break;
	case 3:
		maxOption = 0;
		break;
	}
	menuInput();
	renderMenus();
	delay(LOOP_DELAY);
}

//Render game////////////////////////////////////////////////////////////////////////////////////////////////////////////

void renderMap()
{
	int x, y;
	char c;

	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			c = map[y][x];

			switch (c)
			{
			case '#':
				attron(COLOR_PAIR(1));
				mvaddch(y, x, '#');
				attroff(COLOR_PAIR(1));
				break;
			case ' ':
				attron(COLOR_PAIR(3));
				mvaddch(y, x, ' ');
				attroff(COLOR_PAIR(3));
				break;
			case 'M':
			case 'm':
				attron(COLOR_PAIR(2));
				mvaddch(y, x, '0');
				attroff(COLOR_PAIR(2));
				break;
			case 'D':
			case 'd':
				attron(COLOR_PAIR(4));
				mvaddch(y, x, 'H');
				attroff(COLOR_PAIR(4));
				break;
			case 'A':
			case 'a':
				attron(COLOR_PAIR(9));
				mvaddch(y, x, 'H');
				attroff(COLOR_PAIR(9));
				break;
			case 'B':
			case 'b':
				attron(COLOR_PAIR(6));
				mvaddch(y, x, '/');
				attroff(COLOR_PAIR(6));
				break;
			case 'C':
			case 'c':
				attron(COLOR_PAIR(2));
				mvaddch(y, x, 'F');
				attroff(COLOR_PAIR(2));
				break;
			default:
				mvaddch(y, x, ' ');
			}
		}
	}
}

void renderOgres()
{
	int i;
	for (i = 0; i < totalOgres; i++)
	{
		if (ogres[i].alive)
		{
			attron(COLOR_PAIR(5));
			mvaddch(ogres[i].position.y, ogres[i].position.x, '8');
			attroff(COLOR_PAIR(5));
		}
	}
}

void renderPlayer()
{
	if (swordTime > 3)
	{
		attron(COLOR_PAIR(8));
		mvaddch(playerPosition.y , playerPosition.x, ACS_PLMINUS);
		attroff(COLOR_PAIR(8));
	}
	else
	{
		if (((int)(swordTime * 4)) % 2)
		{
			attron(COLOR_PAIR(8));
			mvaddch(playerPosition.y, playerPosition.x, ACS_PLMINUS);
			attroff(COLOR_PAIR(8));
		}
		else
		{
			attron(COLOR_PAIR(3));
			mvaddch(playerPosition.y, playerPosition.x, ACS_PLMINUS);
			attroff(COLOR_PAIR(3));
		}
	}
}

void renderUI()
{
	mvprintw(1, 30, "Lives: %d", playerLives);
	mvprintw(2, 30, "Coins: %d", playerCoins);
	mvprintw(3, 30, "Keys: %d", playerKeys);
	mvprintw(4, 30, "Time: %0.0f", elapsedTimeS);
}

void renderGame()
{
	clear();
	renderMap();
	renderOgres();
	renderPlayer();
	renderUI();
	refresh();
}

//AI////////////////////////////////////////////////////////////////////////////////////////////////////////////

int testDirection(int x,int y)
{
	int r;
	switch (map[y][x])
	{
	case ' ':
	case 'c':
	case 'a':
	case 'm':
		r = 0;
		break;
	default:
		r = 1;
		break;
	}
	return r;
}

int moveOgre(int ogreID,int x,int y)
{
	int r;
	if (testDirection(ogres[ogreID].position.x + x, ogres[ogreID].position.y + y) == 0)
	{
		ogres[ogreID].position.x += x;
		ogres[ogreID].position.y += y;
		r = 0;
	}
	else
	{
		r = 1;
	}
	return r;
}

void ogresMovement()
{
	int i, rnd;
	for (i = 0; i < totalOgres; i++)
	{
		if (ogres[i].alive)
		{
			rnd = rand() % 4;
			switch (ogres[i].dir)
			{
			case 0://Para cima
				if (moveOgre(i, 0, -1) == 1)
				{
					ogres[i].dir = rnd;
				}
				break;
			case 1:
				if (moveOgre(i, 0, 1) == 1)
				{
					ogres[i].dir = rnd;
				}
				break;
			case 2:
				if (moveOgre(i, 1, 0) == 1)
				{
					ogres[i].dir = rnd;
				}
				break;
			case 3:
				if (moveOgre(i, -1, 0) == 1)
				{
					ogres[i].dir = rnd;
				}
				break;
			}
		}
	}
}

void processAI()
{
	if (elapsedTimeS > lastMovTime + AI_MOVE_DELAY)
	{
		ogresMovement();
		lastMovTime = elapsedTimeS;
	}
}

//Gameplay////////////////////////////////////////////////////////////////////////////////////////////////////////////

void switchDoors()
{
	int x, y;
	for (y = 0; y < MAP_HEIGHT; y++)
	{
		for (x = 0; x < MAP_WIDTH; x++)
		{
			char pos = map[y][x];
			switch (pos)
			{
			case 'D':
			case 'd':
				map[y][x] = 'a';
				break;
			case 'A':
			case 'a':
				map[y][x] = 'd';
				break;
			default:
				break;
			}
		}
	}
}

void testForSwitch()
{
	if (tolower(map[playerPosition.y + 1][playerPosition.x]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[playerPosition.y - 1][playerPosition.x]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[playerPosition.y][playerPosition.x + 1]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[playerPosition.y][playerPosition.x - 1]) == 'b')
	{
		switchDoors(map);
		return;
	}
}


void moveInput()
{
	int key = getch();

	if (key != ERR)
	{
		switch (key)
		{
		case KEY_UP:
		case 'W':
		case 'w':
			nextPosition.y -= 1;
			break;
		case KEY_DOWN:
		case 'S':
		case 's':
			nextPosition.y += 1;
			break;
		case KEY_LEFT:
		case 'A':
		case 'a':
			nextPosition.x -= 1;
			break;
		case KEY_RIGHT:
		case 'D':
		case 'd':
			nextPosition.x += 1;
			break;
		case 'B':
		case 'b':
			testForSwitch();
			break;
		case '\t':
			paused = 1;
		}
	}
}

void tryAndMove()
{
	char nextC = map[nextPosition.y][nextPosition.x];

	switch (nextC)
	{
	case ' ':
	case 'a':
		playerPosition.x = nextPosition.x;
		playerPosition.y = nextPosition.y;
		break;
	case 'c':
		playerPosition.x = nextPosition.x;
		playerPosition.y = nextPosition.y;
		dlKeys[playerKeys].x = nextPosition.x;
		dlKeys[playerKeys].y = nextPosition.y;
		map[nextPosition.y][nextPosition.x] = ' ';
		playerKeys++;
		break;
	case 'm':
		playerPosition.x = nextPosition.x;
		playerPosition.y = nextPosition.y;
		dlCoins[playerCoins].x = nextPosition.x;
		dlCoins[playerCoins].y = nextPosition.y;
		map[nextPosition.y][nextPosition.x] = ' ';
		playerCoins++;
		break;
	default:
		break;
	}
}

void gameOver(int won)
{
	if (won)
	{
		win = 1;
		currentMenu = 2;
		paused = 1;
	}
	else
	{
		win = 0;
		currentMenu = 2;
		paused = 1;
	}
}

void killPlayer()
{
	if (playerLives > 1)
	{
		playerPosition.x = playerSpawn.x;
		playerPosition.y = playerSpawn.y;
		playerLives--;
	}
	else
	{
		gameOver(0);
	}
}

void gameplayManager()
{
	int i;

	if (playerKeys == totalKeys)
	{
		gameOver(1);
	}

	for (i = 0; i < totalOgres; i++)
	{
		if (ogres[i].alive)
		{
			if (ogres[i].position.x == playerPosition.x && ogres[i].position.y == playerPosition.y)
			{
				if (swordTime > 0)
				{
					ogres[i].alive = 0;
					ogresAlive--;
				}
				else
				{
					killPlayer();
				}
			}
		}
	}

	if (playerCoins == totalCoins)
	{
		swordTime = 10;
	}
	else if (playerCoins > lastCoins)
	{
		lastCoins = lastCoins + 10;
		swordTime += SWORD_INC;
	}
	if (swordTime > 0)
	{
		swordTime = max(swordTime - LOOP_DELAY, 0);
	}

}

void gameLoop()
{
	nextPosition.x = playerPosition.x;
	nextPosition.y = playerPosition.y;

	moveInput();
	tryAndMove();
	processAI();
	gameplayManager();
	renderGame();

	timer();
	delay(LOOP_DELAY);
}




int main()
{
	initscr();
	start_color();
	noecho();
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	initPairs();
	timeout(0);

	while (running == 0)
	{
		menuManager();
	}

	while (running)
	{
		if (paused)
		{
			menuManager();
		}
		else
		{
			gameLoop();
		}
	}
}