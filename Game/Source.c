#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define LOOP_DELAY 0.033334f//Quanto tempo demora para um loop completar enquanto o jogo está rodando
#define height 11
#define width 27
#define SWORD_TIME 8

//AI
typedef struct _ogre//Definindo struct ogro
{
	int posX;
	int posY;//Posição do ogro
	bool alive;//Se o ogro está vivo
} ogre;


float AI_MOVE_DELAY = 0.4f;//Tempo que demora para a AI se mover
float AI_SHUFFLE_DELAY = 2.0f;//Temp que demora para a AI dar shuffle (troca quais AI podem se mover ao mesmo tempo.)

float aiMoveStart = 0;//Temporizador do movimento da AI (para comparação com o elapsedTimeS)
float aiShuffleStart = 0;//Temporizador para shuffle (para comparação com o elapsedTimeS)

int totalOgrs = 0;//Ogros totais contidos no mapa
ogre *ogres;

//Game state
int running = 0;//Se o jogo começou
int paused = 0;//Se o jogo está pausado


//Time
float elapsedTimeS = 0;//Tempo decorrido em jogo (não em menu de pausa)
float timeLimit = 60;
/*
clock_t timeBG;
clock_t timeBGG;
clock_t timeED;
float elapsedTimeSGame = 0;
float lastTime = 0;
float deltaTime = 0;*/

//stats
int totalCoins = 0;//Moedas totais no mapa
int totalKeys = 0;//Chaves totais no mapa

int playerCoins = 0, playerKeys = 0;

int lives = 3;

float swordTimer = 0;//Tempo restante com a espada
int lastCoins = 9;//Número de moedas - 1 para ganhar a espada
int spawnY;//Y do spawn do player para fazer respawn
int spawnX;//X do spawn do player para fazer respawn

int difficulty = 1;//Dificuldade
int maxOgres;//Máximo de ogros que podem existir no mapa (depende da dificuldade)

//Menus
int currentMenu = 0;//Menu atual
int selectedOption;//Opção selecionada no menu atual
int maxOpt = 2;//Opções máximas do menu atual (para limitar selected Option)

//////////////////////////////////////////////////////////////

void delay(float secs)//Função de delay (utilizada para travar framerate)
{
	int millis = (int)(1000 * secs);
	clock_t startTime = clock();
	while (clock() < startTime + millis);
}

void incrementClock()//Função que incrementa o elapsedTimeS
{
	elapsedTimeS += LOOP_DELAY;
}

/*void setDelta()//Funções deprecadas
{
	deltaTime = LOOP_DELAY;
	lastTime = elapsedTimeS;
}

void printMap()
{
	char *fileName = malloc(30 * sizeof(char));
	char buff[30];
	int i = 4;
	FILE *mapFilePointer;
	strcpy_s(fileName, 30, "Map01.txt");

	if (fopen_s(&mapFilePointer, fileName, "r") != 0)
	{
		exit(1337);
	}

	while (fgets(buff, 30, mapFilePointer) != NULL)
	{
		printf("%s", buff);
		i--;
	}
}*/

//////////////////////////////////////NPCS////////////////////////////////////////////

void initializeOgre(int id, int x, int y)//Inicializa os valores do ogro
{
	ogres[id].alive = 1;//Inicialmente todos o ogros estão vivos
	ogres[id].posX = x;
	ogres[id].posY = y;//Inicializando a posição dos ogros
}

bool canMoveToTile(char tile)//Retorna se ogro pode se mover para o espaço com o tipo indicado (tipo sendo 'c' para chave '#' para parede, etc.)
{
	switch (tile)
	{
	case ' ':
	case 'm':
	case 'c':
	case 'a':
		return TRUE;
	default:
		return FALSE;
	}
}

void ogreMove(int id, char map[][width])
{
	//Move o ogro para uma direção aleatória
	int dir = rand() % 4;//Aleatoriza a dir
	char tile;//Declarando tile, que vai ser a char que representa o espaço
	switch (dir)
	{
	case 0://Mover para cima
		tile = map[ogres[id].posY - 1][ogres[id].posX];
		if (canMoveToTile(tile))
		{
			ogres[id].posY -= 1;//Decrementa o Y do ogro, pois Y incrementa para baixo
		}
		break;
	case 1://Mover para baixo
		tile = map[ogres[id].posY + 1][ogres[id].posX];
		if (canMoveToTile(tile))
		{
			ogres[id].posY += 1;//Incrementa o Y do ogro, pois Y incrementa para baixo
		}
		break;
	case 2://Mover para a esquerda
		tile = map[ogres[id].posY][ogres[id].posX - 1];
		if (canMoveToTile(tile))
		{
			ogres[id].posX -= 1;//Decrementa X do ogro
		}
		break;
	case 3:
		tile = map[ogres[id].posY][ogres[id].posX + 1];
		if (canMoveToTile(tile))
		{
			ogres[id].posX += 1;//Incrementa X do ogro
		}
		break;
	}
}

void shuffleMoveN(int *movN)
{
	//Aleatoriza numero de movimento, de 0 a 2
	int i;
	for (i = 0; i < totalOgrs; i++)
	{
		movN[i] = rand() % 3;
	}
}

void ogreMover(int *movN, char map[height][width])
{
	//Decide quando mover os ogros
	int i;
	if (elapsedTimeS > (aiShuffleStart + AI_SHUFFLE_DELAY))
	{
		//Depois de AI_SHUFFLE_DELAY segundos, aleatoriza o numero de movimento de um ogro
		shuffleMoveN(movN);
		aiShuffleStart = elapsedTimeS;//Atualiza tempo de inicio para comparação
	}
	
	for (i = 0; i < totalOgrs; i++)
	{
		//Para cada ogro
		if (movN[i] == (rand() % 3))
		{
			//Se um numero aleatório de 0 a 2 é igual seu número de movimento movN[indice do ogro]
			ogreMove(i, map);//Chama a função que move o ogro
		}
	}
}

void moveAI(char map[][width],int *movN)
{
	//Move a AI cada vez que AI_MOVE_DELAY segundos passa
	if (elapsedTimeS > (aiMoveStart + AI_MOVE_DELAY))
	{
		ogreMover(movN, map);
		aiMoveStart = elapsedTimeS;//Reseta o tempo de começo para comparação
	}
}

////////////////////////////MOVEMENT AND INTERACTION//////////////////////////////////
void mapSetup(char map[][width], char *fileName,int *playerX, int *playerY)
{
	//Faz o setup do mapa e entidades
	char *textLine = calloc(width, sizeof(char));//Aloca espaço para a linha de texto que vai ser lida do documento txt
	int line = 0, i = 0, player = 0, ogreID = 0;//Variaveis de controle
	FILE *mapFilePointer;//Ponteiro para o arquivo txt

	if (fopen_s(&mapFilePointer, fileName, "r") != 0)//Tenta abrir o arquivo de mapa
	{
		//Se não conseguir abrir
		exit(1337);//Implementar erro----------------------------
	}

	for (line = 0; line <= height; line++)
	{
		//Para cada linha
		if (line < height)
		{
			//Se não for a ultima linha
			fgets(textLine, width, mapFilePointer);//Pega uma linha do arquivo e manda para textLine
			if (textLine[1] == '\n')
			{
				//Remove a linha com apenas ultimo elemento da linha e \n e adiciona o ultimo elemento da linha à linha anterior
				line--;
				map[line][width - 1] = textLine[0];
				continue;
			}

			for (i = 0; i < width; i++)
			{
				//Poe a linha em lower case
				textLine[i] = tolower(textLine[i]);
			}
			strcpy_s(map[line], width, textLine);//Copia a linha do textLine para a linha do mapa
		}
		else
		{
			//Se é ultima linha
			map[height - 1][width - 1] = textLine[0];//Poe primeiro elemento da linha no ultimo elemento do mapa
		}
	}

	for (line = 0; line < height; line++)
	{
		for (i = 0; i < width; i++)
		{
			//Para cada elemento do mapa
			char c = map[line][i];
			switch (c)
			{
			case 'a':
			case 'd':
				break;
			case 'o':
				totalOgrs++;//Incrementa numero de ogros
				break;
			case 'c':
				totalKeys++;//Incrementa numero de chaves
				break;
			case '#':
				break;
			case 'm':
				totalCoins++;//Incrementa numero de chaves
				break;
			case 'b':
				break;
			case 'j':
				if (player == 0)//Se o spawn do player não foi atribuido
				{
					*playerY = line;
					*playerX = i;//Atribuir posição inicial do player
					spawnX = i;
					spawnY = line;//Atribuir spawn do player
					player = 1;//Atribuir 1, pois o spawn foi atribuido
				}
				map[line][i] = ' ';//Spawn do player vira ' ', que é chão
				break;
			default:
				map[line][i] = ' ';//Caso seja caractere inválida, transformar em chão
				break;
			}
		}
	}
	ogres = calloc(totalOgrs, sizeof(ogre));//Alocar espaço para os ogros, dependendo de quantos existem no mapa

	for (line = 0; line < height; line++)//Incializa os ogros
	{
		for (i = 0; i < width; i++)
		{
			//Para cada elemento do mapa
			char c = map[line][i];//atribuir elemento atual do mapa a c
			switch (c)
			{
			case 'o'://Inicializar ogros caso c seja o
				initializeOgre(ogreID, i, line);
				ogreID++;
				map[line][i] = ' ';//Posição do ogro vira ' ', chão
				break;
			default:
				break;
			}
		}
	}



	fclose(mapFilePointer);//Fecha o arquivo
	free(textLine);//Libera espaço alocado para textLine
}

int countOgresAlive()
{
	//Conta quantos ogros estão vivos no array
	int i;
	int ogresAlive = 0;
	for (i = 0; i < totalOgrs; i++)
	{
		if (ogres[i].alive)
		{
			ogresAlive++;
		}
	}
	return ogresAlive;//Retorna quantos ogros estão vivos
}

void resizeOgrs()
{
	//Redimensiona o array de ogros
	int i, buffLoc = 0, newSize = countOgresAlive();//Buff loc é o local do buffer atual, newSize é o tamanho novo do array que vai ser redimensionado
	ogre *ogrBuffer = malloc(newSize * sizeof(ogre));//Aloca espaço para buffer com tamanho de quantos ogros estão vivos vezes tamanho do struct ogre
	
	for (i = 0; i < totalOgrs; i++)
	{
		if (ogres[i].alive)
		{
			//Copia ogro para buffer se ele estiver vivo
			ogrBuffer[buffLoc] = ogres[i];
			buffLoc++;
		}
	}

	realloc(ogres, newSize * sizeof(ogre));//Redimensiona ogrs para manter endereço inicial e é mais rápido do que alocar um array novo

	for (i = 0; i < newSize; i++)
	{
		ogres[i] = ogrBuffer[i];//Copia ogrBuffer para ogrs
		//ogrs[i].alive = ogrBuffer[i].alive;
		//ogrs[i].posX = ogrBuffer[i].posX;
		//ogrs[i].posY = ogrBuffer[i].posY;
	}
	totalOgrs = newSize;//Atribui newSize para totalOgrs
	free(ogrBuffer);//Libera ogrBuffer da memoria
}

void adjustDifficulty()
{
	switch (difficulty)
	{
		//Ajusta o valor máximo de ogros, dependendo da dificuldade escolhida
	case 0:
		maxOgres = 2;
		break;
	case 1:
		maxOgres = 4;
		break;
	case 2:
		maxOgres = 8;
		break;
	}

	int i = 0, ogresAlive, ogreToKill;
	ogresAlive = countOgresAlive(ogres);//Atribui o numero de ogros vivos para ogresAlive
	while (maxOgres < ogresAlive)//Enquanto o número de ogros vivos é maior do que o número maximo de ogros para essa dificuldade
	{
		ogresAlive = countOgresAlive(ogres);//Atualizar numero de ogros vivos
		ogreToKill = rand() % totalOgrs;//Pegar um ogro aleatório para remover
		for (i = 0; i < totalOgrs; i++)
		{
			if (i == ogreToKill)
			{
				ogres[i].alive = 0;//Se o ogro para remover for igual ao indice do atual, mata-lo
			}
		}
	}
	resizeOgrs();//Redimensionar array de ogros para conter só ogros vivos
}

void gameOver()
{
	paused = 1;
	currentMenu = 2;
}

void playerDie(int *playerX, int *playerY)
{
	//Quando o player morre
	*playerX = spawnX;
	*playerY = spawnY;//Resetar posição
	if (lives > 1)
	{
		lives -= 1;//Se tiver mais de uma vida, deduzir uma vida
	}
	else
	{
		gameOver();//Se não, game over
	}
}

void gameplayManager(char *map, int *playerX, int *playerY)
{
	int i;

	if (playerKeys == totalKeys)
	{
		gameOver();
	}

	if (timeLimit - elapsedTimeS <= 0)
	{
		gameOver();
	}

	for (i = 0; i < totalOgrs; i++)
	{
		//Itera pelos ogros
		if (ogres[i].alive)
		{
			//Se o ogros está vivo
			if (ogres[i].posX == *playerX && ogres[i].posY == *playerY)
			{
				//Se o ogro está na mesma posição do player
				if (swordTimer > 0)
				{
					//Se o player tem a espada, matar ogro
					ogres[i].alive = 0;
				}
				else
				{
					//Se o player não tem a espada, matar player
					playerDie(playerX, playerY);
				}
			}
		}
	}


	if (playerCoins == totalCoins)
	{
		//Se o player pegou todas as modedas, manter o timer de espada em 5000;
		swordTimer = 5000;
	}
	else
	{
		//Se o player não pegou todas as moedas...
		if (playerCoins > lastCoins)
		{
			//Se o player pegou o suficiente de moedas
			lastCoins = playerCoins + 9;//Aumentar o numero de moedas necessárias por 10 (>9)
			swordTimer += SWORD_TIME;//Incrementar tempo com a espada por SWORD_TIME
		}
		if (swordTimer > 0)
		{
			//Se o player está com a espada, decrementar timer da espada por LOOP_DELAY, com limite em 0
			swordTimer = max(0, swordTimer - LOOP_DELAY);
		}
	}
}

void switchDoors(char map[][width])
{
	int x, y;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
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

void testForSwitch(char map[height][width], int x, int y)
{
	if (tolower(map[y + 1][x]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[y - 1][x]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[y][x + 1]) == 'b')
	{
		switchDoors(map);
		return;
	}
	if (tolower(map[y][x - 1]) == 'b')
	{
		switchDoors(map);
		return;
	}
}

void handleInput(int *x, int *y,char map[height][width])
{
	int key = getch();

	if (key != ERR)
	{
		switch (key)
		{
		case KEY_UP:
		case 'w':
			*y -= 1;
			break;
		case KEY_DOWN:
		case 's':
			*y += 1;
			break;
		case KEY_LEFT:
		case 'a':
			*x -= 1;
			break;
		case KEY_RIGHT:
		case 'd':
			*x += 1;
			break;
		case 'b':
			testForSwitch(map, *x ,*y);
			break;
		case '\t':
			paused = 1;
		}
	}
}

void tryAndMove(int nextX, int nextY, int *x, int *y,char map [height][width])
{
	if (nextX >= width || nextX < 0 || nextY >= height || nextY < 0)
	{
		//exit(42);
		return;
	}
	char movePos = map[nextY][nextX];
	switch (movePos)
	{
	case '#':
	case 'd':
	case 'D':
	case 'b':
	case 'B':
		break;
	case 'm':
	case 'M':
		playerCoins += 1;
		map[nextY][nextX] = ' ';
		*x = nextX;
		*y = nextY;
		break;
	case 'C':
	case 'c':
		playerKeys += 1;
		map[nextY][nextX] = ' ';
		*x = nextX;
		*y = nextY;
		break;
	default:
		*x = nextX;
		*y = nextY;
	}
}

////////////////////////////////////////////GWAPHIX////////////////////////////////////////////////

void drawMap(char map[height][width])
{
	int y, x;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			switch (map[y][x])
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
	attroff(1);
}

void drawPlayer(int x, int y)
{
	if (swordTimer > 3)
	{
		attron(COLOR_PAIR(8));
		mvaddch(y, x, ACS_PLMINUS);
		attroff(COLOR_PAIR(8));
	}
	else
	{
		if (((int)(swordTimer * 4)) % 2)
		{
			attron(COLOR_PAIR(8));
			mvaddch(y, x, ACS_PLMINUS);
			attroff(COLOR_PAIR(8));
		}
		else
		{
			attron(COLOR_PAIR(3));
			mvaddch(y, x, ACS_PLMINUS);
			attroff(COLOR_PAIR(3));
		}
	}
}

void drawHUD()
{
	mvprintw(0, width + 2, "Coins: %d/%d", playerCoins, totalCoins);
	mvprintw(1, width + 2, "Lives: %d/3", lives);
	mvprintw(2, width + 2, "Keys: %d/%d", playerKeys, totalKeys);
	mvprintw(3, width + 2, "Time: %0.2f", max(timeLimit - elapsedTimeS, 0));
}

void renderAI()
{
	int i;
	for (i = 0; i < totalOgrs; i++)
	{
		if (ogres[i].alive == TRUE)
		{
			attron(COLOR_PAIR(5));
			mvaddch(ogres[i].posY , ogres[i].posX, '8');
			attroff(COLOR_PAIR(5));
		}
	}
}

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

/////////////////////////////////////////////MENU////////////////////////////////////////////////

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

void firstMenuR()
{
	activateMenuHighlight(0);
	mvprintw(1, 2, "New Game");
	deactivateMenuHighlight();

	activateMenuHighlight(1);
	mvprintw(3, 2, "Options");
	deactivateMenuHighlight();

	activateMenuHighlight(2);
	mvprintw(5, 2, "Quit");
	deactivateMenuHighlight();
}

void pauseMenuR()
{
	activateMenuHighlight(0);
	mvprintw(1, 2, "Resume Game");
	deactivateMenuHighlight();

	activateMenuHighlight(1);
	mvprintw(3, 2, "Save");
	deactivateMenuHighlight();

	activateMenuHighlight(2);
	mvprintw(5, 2, "Quit");
	deactivateMenuHighlight();

	//mvprintw(7, 2, "%0.2f", swordTimer);
}

void optionsMenuR()
{
	mvprintw(1, 2, "Difficuty");

	activateMenuHighlight(0);
	mvprintw(3, 2, "Easy");
	deactivateMenuHighlight();

	activateMenuHighlight(1);
	mvprintw(5, 2, "Medium");
	deactivateMenuHighlight();

	activateMenuHighlight(2);
	mvprintw(7, 2, "Hard");
	deactivateMenuHighlight();
}

void gameOverScreenR()
{
	int killedOgres = totalOgrs - countOgresAlive();
	int score = (playerKeys * 2 + playerCoins + killedOgres * 2 + lives * 3 + (timeLimit - elapsedTimeS))*(difficulty + 1);
	attron(COLOR_PAIR(10));
	mvprintw(1, 2,"You Win!");
	attroff(COLOR_PAIR(10));

	attron(COLOR_PAIR(11));
	mvprintw(3, 2, "Time: %0.2f seconds", elapsedTimeS);
	mvprintw(5, 2, "Points %d", score);
	attroff(COLOR_PAIR(11));

	activateMenuHighlight(0);
	mvprintw(7, 2, "Reset");
	deactivateMenuHighlight();

	activateMenuHighlight(1);
	mvprintw(9, 2, "Quit");
	deactivateMenuHighlight();
}

void drawMenu()
{
	clear();
	switch (currentMenu)
	{
	case 0:
		if (running)
		{
			pauseMenuR();
		}
		else
		{
			firstMenuR();
		}
		break;
	case 1:
		optionsMenuR();
		break;
	case 2:
		gameOverScreenR();
	}
	refresh();
}

void interactWithMenu()
{
	switch (currentMenu)
	{
	case 0:
		if (running)
		{
			switch (selectedOption)
			{
			case 0:
				paused = 0;
				break;
			case 1:
				break;
			case 2:
				paused = 0;
				running = 0;
				break;
			}
		}
		else
		{
			switch (selectedOption)
			{
			case 0:
				running = 1;
				break;
			case 1:
				currentMenu = 1;
				break;
			case 2:
				exit(0);
				break;
			}
		}
		break;
	case 1:
		switch (selectedOption)
		{
		case 0:
			difficulty = 0;
			break;
		case 1:
			difficulty = 1;
			break;
		case 2:
			difficulty = 2;
			break;
		}
		selectedOption = 0;
		currentMenu = 0;
		break;
	case 2:
		switch (selectedOption)
		{
		case 0:
			break;
		case 1:
			paused = 0;
			running = 0;
		}
	}
}

void menuInputManager()
{
	int input = getch();

	if (input != NULL)
	{
		/*switch (currentMenu)
		{
		case 0://Main/Pause Menu
		case 1://Options menu
			maxOpt = 2;
			break;
		case 2://Win menu
			maxOpt = 1;
			break;
		}*/

		switch (input)
		{
		case ' ':
		case '\n':
			interactWithMenu();
			break;
		case KEY_DOWN:
			selectedOption = min(maxOpt, selectedOption + 1);
			break;
		case KEY_UP:
			selectedOption = max(0, selectedOption - 1);
			break;
		case '\t':
			paused = 0;
		}
	}
}

void menuManager()
{
	menuInputManager();
	drawMenu();
}

/////////////////////////////////////////GAME/////////////////////////////////////////////

void gameLoop(char map[height][width], int *x, int *y, int *ogreMoveN, int *nextX, int *nextY)
{
	incrementClock();
	clear();

	drawMap(map);
	drawPlayer(*x, *y);
	drawHUD();
	renderAI();

	refresh();

	*nextX = *x;
	*nextY = *y;
	handleInput(nextX, nextY, map);
	tryAndMove(*nextX, *nextY, x, y, map);
	moveAI(map, ogreMoveN);
	gameplayManager(map, x, y);

	delay(LOOP_DELAY);
}

int main()
{
	int y = 1, x = 1;
	int nextX, nextY;

	char *mapName;

	char map[height][width]; /*= {
	'#','#','#','#','#','#','#','#','#','#','#','#','#','#','b','#','#','#','#','#','#','#','#','#','#','#','#',
	'#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','m','#',
	'#','#','#','#','#','b','#','#','#','#','#','#','#','#','d','#','#','#','#','#','#','a','#','#',' ','m','#',
	'#',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','a',' ',' ',' ',' ','#','#','c',' ','m','m','m','#',' ','m','#',
	'#',' ','#','#','#','a','#',' ',' ','#',' ','#',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ','m','#',
	'#',' ','#',' ','c',' ','#',' ',' ',' ',' ','b',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','a',' ','m','#',
	'#',' ','#',' ',' ',' ','#','c',' ','#',' ','#',' ',' ',' ',' ','#','#',' ',' ','m','m','m','#',' ','m','#',
	'#',' ','#',' ',' ',' ','#','#','d','#',' ','#','#','#','d','#','#','#','#','#','#','d','#','#',' ','#','#',
	'#',' ','b',' ',' ',' ','#',' ',' ',' ',' ',' ',' ',' ',' ','c','#',' ',' ',' ','m','m','m',' ',' ',' ','#',
	'#',' ',' ',' ',' ',' ','a',' ',' ',' ',' ',' ',' ',' ',' ',' ','#',' ',' ',' ','m','m','m',' ',' ',' ','#',
	'#','#','#','#','#','#','#','#','#','b','#','#','#','#','#','#','#','#','b','#','#','#','#','#','#','#','#'
	};*/

	//printMap();

	//delay(4);

	initscr();
	start_color();
	noecho();
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	initPairs();
	timeout(0);
	//timeBG = clock();

	mapName = malloc(250 * sizeof(char));



	strcpy_s(mapName, 250, "Map01.txt");
	mapSetup(map, mapName, &x, &y);
	int *ogreMoveN = malloc(sizeof(int) * maxOgres);

	while (!running)
	{
		menuManager();
	}

	adjustDifficulty(ogres, &ogres);

	while (running)
	{
		if (paused)
		{
			menuManager();
		}
		else
		{
			gameLoop(map, &x, &y, ogreMoveN, &nextX, &nextY);
		}
	}

	return 0;
}