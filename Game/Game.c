#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAP_HEIGHT 11//Altura do mapa
#define MAP_WIDTH 27//Largura do mapa
#define LOOP_DELAY 0.0333332//Tempo do delay entre os loops
#define SWORD_INC 5//Tempo a incrementar para a espada
#define AI_MOVE_DELAY 1//Tempo entre movimento dos ogros

//Geral

int win = 0;//Se o jogador ganhou

typedef struct//Struct de coordenadas 2d
{
	int x;
	int y;
} vec2;

typedef struct//Struct dos ogros
{
	int alive;
	vec2 position;
	vec2 spawn;
	int dir;//Direção em que o ogro vai se mover
} ogre;

int running = 0;//Se o jogo está rodando
int paused = 0;//Se o jogo está pausado

//Timing
float elapsedTimeS;//Tempo que se passou no jogo

//Player
vec2 playerPosition;//Posição do jogador
vec2 nextPosition;//Posição para que o jogador vai se mover
vec2 playerSpawn;//Local de spawn do jogador
int playerLives;//Vidas do jogador
int playerCoins;//Quantas moedas o jogador tem
int playerKeys;//Chaves que o jogador tem
int lastCoins = 9;//Quantas moedas o jogador deve coletar -1 para ganhar a espada
float swordTime;//Tempo em que o jogador tem a espada

//AI
ogre ogres[4];//Declarando o array de ogros
int totalOgres;//Total de ogros no mapa
int ogresAlive;//Ogros que estão vivos
int lastMovTime = 0;//Tempo em que os ogros se moveram pela última vez

//Map
int mapN = 1;//Numero do mapa atual
char map[MAP_HEIGHT][MAP_WIDTH];//Declarando o array do mapa
vec2 *dlCoins;//Modedas para deletar do mapa quando carregar jogo
vec2 *dlKeys;//Chaves para deletar do mapa quando carregar jogo
int totalKeys;//Chaves totais no mapa
int totalCoins;//Moedas totais no mapa
int doorState = 0;//Estado das portas

//Menus
int selectedOption = 0;//Opção selecionada no menu
int currentMenu = 0;//Menu atual
int maxOption = 2;//Número máximo das opções







//Geral////////////////////////////////////////////////////////////////////////////////////////////////////////////
void delay(float secs)//Função de delay (utilizada para travar framerate)
{
	int millis = (int)(1000 * secs);
	clock_t startTime = clock();
	while (clock() < startTime + millis);
}

void timer()//Função para incrementar tempo de jogo
{
	elapsedTimeS += LOOP_DELAY;
}

void printMap()//Função de debug para printar mapa em texto plano
{
	int i, j;

	for (i = 0; i < MAP_HEIGHT; i++)
	{
		for (j = 0; j < MAP_WIDTH; j++)
		{
			mvaddch(i, j, map[i][j]);
		}
	}
	refresh();
	delay(2);
}


//Carregar/Salvar////////////////////////////////////////////////////////////////////////////////////////////////////////////
int loadMap(int mapId, int resetStats)//Carrega o mapa
{
	int i, j, r = 1;//Variáveis de controle
	int rnd;//Número aleatório para direção dos ogros
	char fileName[14], symbol;//Nome do arquivo a abrir e char a ser carregada
	FILE *mapFile;//Ponteiro para o arquivo de mapa

	sprintf_s(fileName, 14, "mapa%02d.txt", mapId);//Ajeitar nome do mapa na string

	if (fopen_s(&mapFile, fileName, "r") == 0)//Abrir arquivo de mapa
	{

		totalOgres = 0;
		totalCoins = 0;
		totalKeys = 0;
		lastMovTime = 0;
		free(dlCoins);
		free(dlKeys);//Zerando variáveis de gameplay

		if (resetStats)
		{
			playerCoins = 0;//Se resetStats é verdadeiro, resetar stats do player
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
				symbol = fgetc(mapFile);//Iterar pelo arquivo pegando chars e pondo no map
				if (symbol != '\n')
				{
					symbol = tolower(symbol);//Pondo letra em lower case
					map[i][j] = symbol;
				}
			}
		}

		for (i = 0; i < MAP_HEIGHT; i++)
		{
			for (j = 0; j <= MAP_WIDTH; j++)
			{
				//Iterar pelo mapa, substituir 'o' e 'j' e contar moedas e chaves
				symbol = map[i][j];
				switch (symbol)
				{
				case 'o':
					if (totalOgres < 4)
					{
						rnd = rand() % 4;
						ogres[totalOgres].alive = 1;
						ogres[totalOgres].position.x = j;
						ogres[totalOgres].position.y = i;
						ogres[totalOgres].dir = rnd;
						ogres[totalOgres].spawn.x = j;
						ogres[totalOgres].spawn.y = i;
						totalOgres++;
					}
					map[i][j] = ' ';
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
				}
			}
		}
		dlCoins = malloc(sizeof(vec2) * totalCoins);
		dlKeys = malloc(sizeof(vec2) * totalKeys);//Alocando espaços para as moedas e chaves

		ogresAlive = totalOgres;//Ogros vivos são o mesmo número de ogros totais no início

		if (fclose(mapFile) == 0)//Se mapa foi fechado com sucesso
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
	return r;//Retornar r
}

int saveGame(int slot)
{
	FILE *saveFile;//Ponteiro para o arquivo de save
	char saveName[6];//Nome do arquivo de save
	int i, r;//Variáveis de controle

	sprintf_s(saveName, 6, "%d.sav", slot);//Arrumando nome do arquivo dependendo do slot escolhido

	if (fopen_s(&saveFile, saveName, "w") == 0)//Abrindo arquivo para escrita
	{
		fprintf_s(saveFile, "s%d\n", mapN);//Printando no arquivo os dados do jogo, com s indicando status e c indicando coordenadas
		fprintf_s(saveFile, "s%d\n", doorState);
		fprintf_s(saveFile, "c%d,%d\n", playerPosition.x, playerPosition.y);
		fprintf_s(saveFile, "s%d\n", playerCoins);
		fprintf_s(saveFile, "s%d\n", playerLives);
		fprintf_s(saveFile, "s%d\n", playerKeys);
		fprintf_s(saveFile, "s%d\n", (int)elapsedTimeS);
		fprintf_s(saveFile, "s%d\n", (int)swordTime);
		fprintf_s(saveFile, "s%d\n", lastCoins);
		fprintf_s(saveFile, "s%d\n", totalOgres);
		fprintf_s(saveFile, "s%d\n", ogresAlive);
		for (i = 0; i < totalOgres; i++)//Printando no arquivo as posições dos ogros vivos
		{
			if (ogres[i].alive)
			{
				fprintf_s(saveFile, "c%d,%d\n", ogres[i].position.x, ogres[i].position.y);
			}
		}
		for (i = 0; i < playerCoins; i++)//Printando no arquivo as posições das moedas recolhidas
		{
			fprintf_s(saveFile, "c%d,%d\n", dlCoins[i].x, dlCoins[i].y);
		}
		for (i = 0; i < playerKeys; i++)//Printando no arquiovo as posições das chaves recolhidas
		{
			fprintf_s(saveFile, "c%d,%d\n", dlKeys[i].x, dlKeys[i].y);
		}
		if (fclose(saveFile) == 0)//Fechando/salvando o arquivo
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
	return r;//Retornando r = 0 para sucesso ou 1 para falha
}

int extractLine(FILE *save, int *num, int *x, int *y)//Extrai informação de uma linha do arquivo da save
{
	int i, s, type;//Variáveis de controle
	char buff[3] = {"aa"};//Buffer para converter em int
	s = fgetc(save);//Pegando primeiro char da linha
	switch (s)
	{
	case 's'://Se for statistica, ler até newline, pondo no buffer as caracteres
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
		*num = atoi(buff);//Converter buffer para int e por em num
		*x = -1;
		*y = -1;
		break;
	case 'c'://Se for coordenada, ler até virgula, pondo no buffer as caracteres
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
		*x = atoi(buff);//Converter buffer em int e por em x
		sprintf_s(buff, 3, "aa");//Limpar buffer de números
		for (i = 0; i < 3; i++)//Ler até newline, pondo no buffer as caracteres
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
		*y = atoi(buff);//Converter buffer em int e por em y
		*num = -1;
		break;
	default:
		type = -1;
		break;
	}
	return type;//Retornar tipo de dado
}

void switchDoors();

int loadGame(int slot)
{
	FILE *saveFile;//Ponteiro para o arquivo de save
	char saveName[6];//Nome do save
	int i, r = 1;//Variáveis de controle
	int num, x, y;//Variáveis que serão retornadas por extractLine
	
	sprintf_s(saveName, 6, "%d.sav", slot);//Arrumer nome do save

	if (fopen_s(&saveFile, saveName, "r") == 0)//Abrir mapa
	{
		extractLine(saveFile, &num, &x, &y);//Extrair dados linha por linha
		mapN = num;
		loadMap(mapN, 0);
		extractLine(saveFile, &num, &x, &y);
		doorState = num;
		if (doorState)
		{
			switchDoors();
		}
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
		for (i = 0; i < totalOgres; i++)//Setup dos ogros
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

		for (i = 0; i < playerCoins; i++)//Carregando moedas a deletar do mapa e deletando-as
		{
			extractLine(saveFile, &num, &x, &y);
			dlCoins[i].x = x;
			dlCoins[i].y = y;
			map[y][x] = ' ';
		}

		for (i = 0; i < playerKeys; i++)//Carregando chaves a deletar do mapa e deletando-as
		{
			extractLine(saveFile, &num, &x, &y);
			dlKeys[i].x = x;
			dlKeys[i].y = y;
			map[y][x] = ' ';
		}
		fclose(saveFile);//Fechar save
		r = 0;
	}
	else
	{
		r = 1;
	}
	return r;//Retornado se arquivo foi carregado com sucesso
}

//Menus////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initPairs()//Pares de cor para gráficos
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

void activateMenuHighlight(int option)//Ativar highlight na opção desejada
{
	if (option == selectedOption)
	{
		attron(COLOR_PAIR(7));
	}
}

void deactivateMenuHighlight()//Desativar highlight
{
	attroff(COLOR_PAIR(7));
}


void mainMenuR()//ID 0
{
	if (running == 0)//Se o jogo não está rodando mostrar menu principal
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
	else//Se o jogo está rodando, mostrar menu de pausa
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
	if (running)//Se o jogo está rodando, mostrar menu de save
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
	else//Se o jogo não está rodando, mostrar menu de load
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

void endingR()//Fim de todos os mapas
{
	mvprintw(1, 2, "The End");

	activateMenuHighlight(0);
	mvprintw(3, 2, "Quit");
	deactivateMenuHighlight();
}


void endGameR()//Id 2
{
	if (win == 1)//Se jogador ganhou, mostar menu de win
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
	else//Se não, mostar menu lose
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

void renderMenus()//Escolhe qual menu é renderizado
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

interactWithMenu()//Interações com o menu, apenas faz o que a parte visual indica
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
					mvprintw(1, 2, "Could not load save.");//Aviso
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
					mvprintw(1, 2, "Could not load save.");//Aviso
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
					mvprintw(1, 2, "Could not load save.");//Aviso
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
					mvprintw(1, 2, "Could not save game.");//Aviso
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");//Aviso
					refresh();
					delay(1);
					currentMenu = 0;
				}
				break;
			case 1:
				if (saveGame(2) != 0)
				{
					mvprintw(1, 2, "Could not save game.");//Aviso
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");//Aviso
					refresh();
					delay(1);
					currentMenu = 0;
				}
				break;
			case 2:
				if (saveGame(3) != 0)
				{
					mvprintw(1, 2, "Could not save game.");//Aviso
					refresh();
					delay(1);
				}
				else
				{
					mvprintw(1, 2, "Save successful.");//Aviso
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
					if (mapN < 99)
					{
						mapN++;
						if (loadMap(mapN, 1) == 0)
						{
							currentMenu = 0;
							paused = 0;
						}
						else
						{
							currentMenu = 3;
						}
					}
					else
					{
						currentMenu = 3;
					}
				}
				else
				{
					loadMap(mapN, 1);
					currentMenu = 0;
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

void menuInput()//Pega input para o menu, move o cursor
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

void menuManager()//É chamado em loop e diz o número de opções de cada menu
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

void renderMap()//Printa o mapa com cores e caracteres
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

void renderOgres()//Renderiza cada ogro
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

void renderPlayer()//Renderiza o jogador se ele está com invicibilidade ou não sendo um fator
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

void renderUI()//Renderiza a informação do jogo atual na tela
{
	mvprintw(1, 30, "Lives: %d", playerLives);
	mvprintw(2, 30, "Coins: %d", playerCoins);
	mvprintw(3, 30, "Keys: %d", playerKeys);
	mvprintw(4, 30, "Time: %0.0f", elapsedTimeS);
}

void renderGame()//Chama as funções de renderização
{
	clear();
	renderMap();
	renderOgres();
	renderPlayer();
	renderUI();
	refresh();
}

//AI////////////////////////////////////////////////////////////////////////////////////////////////////////////

int testDirection(int x,int y)//Retorna se o ogro pode se mover para a posição informada
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

int moveOgre(int ogreID,int x,int y)//Move o ogro se ele pode se mover e retorna se ele se moveu
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

void ogresMovement()//Move os ogros e muda suas direções se eles não se moveram
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

void processAI()//Temporiza o movimento dos ogros
{
	if (elapsedTimeS > lastMovTime + AI_MOVE_DELAY)
	{
		ogresMovement();
		lastMovTime = elapsedTimeS;
	}
}

//Gameplay////////////////////////////////////////////////////////////////////////////////////////////////////////////

void switchDoors()//Muda o estado das portas no mapa
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
	if (doorState)
	{
		doorState = 0;
	}
	else
	{
		doorState = 1;
	}
}

void testForSwitch()//Testa se tem uma alavanca adjacente ao jogador
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


void moveInput()//Input do jogo, decide para onde o jogador deve se mover e interação com o jogo
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
			break;
		case 'z':
			printMap();
		}
	}
}

void tryAndMove()//Tenta mover o jogador
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

void gameOver(int won)//Termina a partida atual, ganhando ou perdendo
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

void killPlayer()//Mata o jogador e termina a partida se ele está sem vidas
{
	if (playerLives > 1)
	{
		int i;
		playerPosition.x = playerSpawn.x;
		playerPosition.y = playerSpawn.y;
		playerLives--;

		for (i = 0; i < totalOgres; i++)//Reposiciona os ogros
		{
			if (ogres[i].alive)
			{
				ogres[i].position.x = ogres[i].spawn.x;
				ogres[i].position.y = ogres[i].spawn.y;
			}
		}
	}
	else
	{
		gameOver(0);
	}
}

void gameplayManager()//Gerencia o gameplay
{
	int i;

	if (playerKeys == totalKeys)//Jogador ganha se pega todas a moedas
	{
		gameOver(1);
	}

	for (i = 0; i < totalOgres; i++)//Colisão com ogros
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

	if (playerCoins == totalCoins)//Gerencia a espada
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

void gameLoop()//Loop do jogo
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
	initscr();//Inicializa curses
	start_color();//Inicializa cor no console
	noecho();//Não mosta input na tela
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	initPairs();//Inicializa os pares de cores
	timeout(0);//Não espera enter

	while (running == 0)
	{
		menuManager();//Gerencia menus
	}

	while (running)
	{
		if (paused)
		{
			menuManager();//Gerencia menus
		}
		else
		{
			gameLoop();//Loop de jogo
		}
	}
	return 0;
}