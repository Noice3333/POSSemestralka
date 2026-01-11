#include "snake.h"

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
	char* outputBuffer = malloc((MAX_GAME_HEIGHT * (MAX_GAME_WIDTH + 1)) + 1024);
	int currentTime = 0;
	int scores[MAX_PLAYERS];
	while (1) {
		WaitForSingleObject(args->drawArgs->dEvent, INFINITE);
		
		AcquireSRWLockExclusive(args->tickLock);
		if (!args->inputInfo->continueGame) {
			printf("Game will no longer continue.\n");
			ReleaseSRWLockExclusive(args->tickLock);
			break;
		}
		COORD cursorPosition = { 0, 0 };
		SetConsoleCursorPosition(consoleHandle, cursorPosition);
		if (args->inputInfo->mode == MODE_PLAY) {
			if (!args->inputInfo->permissionToConnect) {
				args->inputInfo->mode = MODE_MENU;
			}
			else {
				if (!needToClear) {
					system("cls");
				}
				needToClear = TRUE;
				int pos = 0;
				for (int i = 0; i < args->drawArgs->height; i++) {
					memcpy(outputBuffer + pos, &args->drawArgs->map[i * args->drawArgs->width], args->drawArgs->width);
					pos += args->drawArgs->width;
					outputBuffer[pos++] = '\n';
				}
				outputBuffer[pos] = '\0';
				for (int i = 0; i < MAX_PLAYERS; i++) {
					scores[i] = args->playerScores[i];
				}
				currentTime = args->gameTime;
			}
		}
		if (args->inputInfo->mode == MODE_MENU) {
			if (needToClear) {
				system("cls");
				needToClear = FALSE;
			}
			printf("Menu Mode\nn - New game\nj - Join game\nc - Continue\ne- Exit\n");
		}
		int currentMode = args->inputInfo->mode;
		ReleaseSRWLockExclusive(args->tickLock);
		if (currentMode == MODE_PLAY) {
			printf("%s", outputBuffer);
			printf("\nGame time: %3d", currentTime / 1000);
			for (int i = 0; i < MAX_PLAYERS; i++) {
				int score = scores[i];
				if (score >= 0) {
					printf("\nPlayer %c score: %4d", 'A' + i, score);
				}
				else {
					char empty = ' ';
					printf("\n%20c", empty);
				}
			}
		}
	}
	free(outputBuffer);
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
	for (int i = 0; i < MAX_PLAYERS; i++) {
		Segment* current = args->heads[i];
		if (current != NULL) {
			Segment* last = current;
			int snakeLength = 1;
			int counter = 0;
			_Bool moved = FALSE;
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
						if (!args->heads[i]->isPaused) {
							if (args->heads[i]->pauseMS >= WAIT_UNPAUSE_MS) {
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
											next->isPaused = FALSE;
											next->next = NULL;
											last->next = next;
											args->playerScores[i]++;
										}
									}
									moved = TRUE;
									args->drawArgs->map[current->y * args->drawArgs->width + current->x] = current->segChar;
								}
							}
							else {
								args->heads[i]->pauseMS += TICK_INTERVAL_MS;
							}
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
				args->playerScores[i] = -1;
				deSnake(current);
				args->heads[i] = NULL;
			}
			else {
				if (moved) {
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

