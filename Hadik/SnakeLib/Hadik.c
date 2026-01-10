#include "Hadik.h"

void clear_buffer() {
	int c;
	// Read one char at a time until \n or EOF is reached
	while ((c = getchar()) != '\n' && c != EOF);
}


int draw(void* arg) {
	GameInfo* args = (GameInfo*)arg;

	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);
	_Bool needToClear = TRUE;

	system("cls");

	while (1) {
		WaitForSingleObject(args->drawArgs->dEvent, INFINITE);
		COORD cursorPosition = { 0, 0 };
		SetConsoleCursorPosition(consoleHandle, cursorPosition);
		AcquireSRWLockExclusive(&args->tickLock);
		int* currentMode = &args->inputInfo->mode;
		if (!args->inputInfo->continueGame) {
			system("cls");
			printf("Game was quit.\n");
			ReleaseSRWLockExclusive(&args->tickLock);
			break;
		}
		if (*currentMode == MODE_MENU) {
			if (needToClear) {
				system("cls");
				needToClear = FALSE;
			}
			printf("Menu Mode\nn - New game\nj - Join game\nc - Continue\ne- Exit\n");
		}
		else if (*currentMode == MODE_PLAY) {
			needToClear = TRUE;
			for (int i = 0; i < args->drawArgs->height; i++) {
				for (int j = 0; j < args->drawArgs->width; j++) {
					if (j % args->drawArgs->width == 0) {
						printf("\n");
					}
					printf("%c", args->drawArgs->map[i * args->drawArgs->width + j]);
				}
			}
		}
		ReleaseSRWLockExclusive(&args->tickLock);
	}
	return 0;
}

int inputHandler(void* arg) {
	GameInfo* input = (GameInfo*)arg;
	int* socket = &input->inputInfo->clientSocket;
	char sendThis = ' ';
	char buffer[10];
	while (1) {
		if (_kbhit()) {
			int ch = _getch();
			AcquireSRWLockExclusive(&input->tickLock);
			int* currentMode = &input->inputInfo->mode;
			ReleaseSRWLockExclusive(&input->tickLock);
			if (*currentMode == MODE_MENU) {
				switch (ch) {
				case 'n': case 'N':
					AcquireSRWLockExclusive(&input->tickLock);
					system("cls");
					printf("Starting new game...\n");
					printf("Enter width (min 10, max %d): ", MAX_GAME_WIDTH);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialWidth = DEFAULT_GAME_WIDTH;
					potentialWidth = strtol(buffer, NULL, 10);
					if (potentialWidth > MAX_GAME_WIDTH || potentialWidth < 10) {
						printf("Invalid input for width, defaulting to %d\n", DEFAULT_GAME_WIDTH);
						potentialWidth = DEFAULT_GAME_WIDTH;
					}
					printf("Enter height (min 10, max %d): ", MAX_GAME_HEIGHT);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialHeight = DEFAULT_GAME_HEIGHT;
					potentialHeight = strtol(buffer, NULL, 10);
					if (potentialHeight > MAX_GAME_HEIGHT || potentialHeight < 10) {
						printf("Invalid input for height, defaulting to %d\n", DEFAULT_GAME_HEIGHT);
						potentialHeight = DEFAULT_GAME_HEIGHT;
					}
					printf("Enter number of players (min 1, max %d): ", MAX_PLAYERS);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialNumOfPlayers = DEFAULT_PLAYER_COUNT;
					potentialNumOfPlayers = strtol(buffer, NULL, 10);
					if (potentialNumOfPlayers > MAX_PLAYERS || potentialNumOfPlayers < 1) {
						printf("Invalid input for number of players, defaulting to %d\n", DEFAULT_PLAYER_COUNT);
						potentialNumOfPlayers = DEFAULT_PLAYER_COUNT;
					}
					printf("Enter time limit: (0 = no limit, max 10000): ");
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialTimeLimit = DEFAULT_GAME_TIME;
					potentialTimeLimit = strtol(buffer, NULL, 10);
					if (potentialTimeLimit > MAX_GAME_TIME || potentialTimeLimit < 0) {
						printf("Invalid input for time limit, defaulting to %d\n", DEFAULT_GAME_TIME);
						potentialTimeLimit = DEFAULT_GAME_TIME;
					}
					
					input->inputInfo->newGameHeight = potentialHeight;
					input->inputInfo->newGameWidth = potentialWidth;
					input->inputInfo->newGamePlayerCount = potentialNumOfPlayers;
					input->inputInfo->newGameTimeLimit = potentialTimeLimit;
					input->drawArgs->height = potentialHeight;
					input->drawArgs->width = potentialWidth;

					if (potentialHeight != input->drawArgs->height || potentialWidth != input->drawArgs->width) {
						int reallErrors = 0;
						char* potentialMap = (char*)realloc(input->drawArgs->map, input->drawArgs->height * input->drawArgs->width * sizeof(char*));
						if (potentialMap != NULL) {
							/*
							for (int i = 0; i < input->drawArgs->height; i++) {
								char* potentialRow = (char*)realloc(input->drawArgs->map[i], sizeof(char) * input->drawArgs->width);
								if (potentialRow != NULL) {
									potentialMap[i] = potentialRow;
								}
								else {
									reallErrors++;
									break;
								}
							}
							*/
						}
						else {
							reallErrors++;
						}

						if (reallErrors > 0) {
							printf("Memory allocation failed, cannot start new game.\n");
							sendThis = '0';
							ReleaseSRWLockExclusive(&input->tickLock);
							break;
						}
						else {
							input->drawArgs->map = potentialMap;
						}
					}

					system("pause");
					*currentMode = MODE_PLAY;
					ReleaseSRWLockExclusive(&input->tickLock);
					system("cls");
					sendThis = 'n';
					break;
				case 'j': case 'J':
					sendThis = 'j';
					break;
				case 'c': case 'C':
					sendThis = 'c';
					AcquireSRWLockExclusive(&input->tickLock);
					*currentMode = MODE_PLAY;
					ReleaseSRWLockExclusive(&input->tickLock);
					break;
				case 'e': case 'E':
					sendThis = 'e';
					AcquireSRWLockExclusive(&input->tickLock);
					input->inputInfo->continueGame = FALSE;
					ReleaseSRWLockExclusive(&input->tickLock);
					send(*socket, &sendThis, 1, 0);
					return 0;
				}
			}
			else if (*currentMode == MODE_PLAY) {
				switch (ch) {
				case 'w': case 'W':
					sendThis = 'w';
					//player->direction = DIRS[UP];
					break;
				case 'a': case 'A':
					sendThis = 'a';
					//player->direction = DIRS[LEFT];
					break;
				case 's': case 'S':
					sendThis = 's';
					//player->direction = DIRS[DOWN];
					break;
				case 'd': case 'D':
					sendThis = 'd';
					//player->direction = DIRS[RIGHT];
					break;
				case 'm': case 'M':
					sendThis = 'm';
					AcquireSRWLockExclusive(&input->tickLock);
					*currentMode = MODE_MENU;
					ReleaseSRWLockExclusive(&input->tickLock);
					break;
				default:
					break;
				}
			}
			send(*socket, &sendThis, 1, 0);
			if (sendThis == 'n') {
				send(*socket, (char*)input->inputInfo, sizeof(InputInfo), 0);
			}
		}
		AcquireSRWLockExclusive(&input->tickLock);
		if (!input->inputInfo->continueGame) {
			ReleaseSRWLockExclusive(&input->tickLock);
			break;
		}
		ReleaseSRWLockExclusive(&input->tickLock);
	}
	return 0;
}

void deSnake(void* arg) {
	Segment* head = (Segment*)arg;
	Segment* temp = head;
	Segment* toFree = head;
	while (1) {
		temp = toFree->next;
		free(toFree);
		toFree = temp;
		if (temp == NULL) {
			break;
		}
	}
}

void updateSnake(void* arg) {
	GameInfo* args = (GameInfo*)arg;
	if (args == NULL || !args->isRunning) {
		return;
	}
	for (int i = 0; i < args->inputInfo->newGamePlayerCount; i++) {
		Segment* current = args->heads[i];
		if (current != NULL) {
			Segment* last = current;
			int snakeLength = 1;
			int counter = 0;
			// Find the last segment
			while (last != NULL && last->next != NULL) {
				snakeLength++;
				last = last->next;
			}
			int lastOriginalX = last->x;
			int lastOriginalY = last->y;
			while (counter < snakeLength) {
				if (current != NULL) {
					if (current->isAlive) {
						// Clear current position and move to new position
						args->drawArgs->map[current->y * args->drawArgs->width + current->x] = ' ';
						current->x += current->direction.difX;
						current->y += current->direction.difY;

						// Wrap around logic
						if (current->x == args->drawArgs->width - 1) {
							current->x = 1;
						}
						else if (current->x == 0) {
							current->x = args->drawArgs->width - 2;
						}
						if (current->y == args->drawArgs->height - 1) {
							current->y = 1;
						}
						else if (current->y == 0) {
							current->y = args->drawArgs->height - 2;
						}


						// Check for snake collision
						if (current == args->heads[i] && args->drawArgs->map[current->y * args->drawArgs->width + current->x] != ' ') {
							char collidedChar = args->drawArgs->map[current->y * args->drawArgs->width + current->x];
							switch (collidedChar) {
								case '-':
									break;
								case '|':
									break;
								case '@':
									break;
								default:
									current->isAlive = FALSE;
									args->drawArgs->map[current->y * args->drawArgs->width + current->x] = collidedChar;
									Segment* nextDead = current->next;
									if (nextDead != NULL) {
										nextDead->isAlive = FALSE;
									}
									break;
							}
						}
						if (current->isAlive) {
							// Check for food collision
							for (int j = 0; j < args->inputInfo->newGamePlayerCount; j++) {
								if (current == args->heads[i] && current->x == args->food[j]->x && current->y == args->food[j]->y) {
									Position* newFoodPos = findEmptySpace(args);
									if (newFoodPos != NULL) {
										args->food[j]->x = newFoodPos->x;
										args->food[j]->y = newFoodPos->y;
										//args->food[j]->y += 1;
										args->drawArgs->map[args->food[j]->y * args->drawArgs->width + args->food[j]->x] = args->food[j]->segChar;
										free(newFoodPos);
									}
									Segment* next = (Segment*)malloc(sizeof(Segment));
									next->x = lastOriginalX;
									next->y = lastOriginalY;
									next->segChar = '#';
									next->direction = DIRS[STOP];
									next->isAlive = TRUE;
									next->next = NULL;
									last->next = next;
								}
							}

							
							args->drawArgs->map[current->y * args->drawArgs->width + current->x] = current->segChar;
						}
					}
					else {
						args->drawArgs->map[current->y * args->drawArgs->width + current->x] = ' ';
						Segment* nextDead = current->next;
						if (nextDead != NULL) {
							nextDead->isAlive = FALSE;
						}
					}
				}

				current = current->next;
				counter++;
			}
			current = args->heads[i];
			if (!current->isAlive) {
				deSnake(current);
				args->heads[i] = NULL;
			}
			else {
				Segment* nextGuy;
				Direction tempDirs[2];
				_Bool tempDirSwapper = 0;
				tempDirs[0] = current->direction;
				while (current != NULL && current->isAlive && current->next != NULL) {
					nextGuy = current->next;

					tempDirs[!tempDirSwapper] = nextGuy->direction;
					nextGuy->direction = tempDirs[tempDirSwapper];

					tempDirSwapper = !tempDirSwapper;
					current = current->next;
				}
			}
		}
	}
}

Position* findEmptySpace(void* arg) {
	GameInfo* args = (GameInfo*)arg;
	int attempts = 0;
	while (attempts < 100) {
		int randomWide = (rand() % (args->drawArgs->width - 2)) + 1;
		int randomHeight = (rand() % (args->drawArgs->height - 2)) + 1;
		if (args->drawArgs->map[randomHeight * args->drawArgs->width + randomWide] == ' ') {
			
			Position* pos = (Position*)malloc(sizeof(Position));
			pos->x = randomWide;
			pos->y = randomHeight;
			return pos;
		}
		attempts++;
	}
	return NULL;
}

