#include "Hadik.h"


int draw(void* arg) {
	GameInfo* args = (GameInfo*)arg;

	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);


	while (1) {
		WaitForSingleObject(args->drawArgs->dEvent, INFINITE);
		COORD cursorPosition = { 0, 0 };
		SetConsoleCursorPosition(consoleHandle, cursorPosition);
		AcquireSRWLockExclusive(&args->tickLock);
		for (int i = 0; i < args->drawArgs->height; i++) {
			for (int j = 0; j < args->drawArgs->width; j++) {
				printf("%c", args->drawArgs->map[i][j]);
				if (j == args->drawArgs->width - 1) {
					printf("\n");
				}
			}
		}
		ReleaseSRWLockExclusive(&args->tickLock);
	}
	return 0;
}

int inputHandler(void* arg) {
	int* socket = (int*)arg;
	char sendThis = ' ';
	while (1) {
		if (_kbhit()) {
			int ch = _getch();
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
			case 'q': case 'Q':
				sendThis = 'q';
				printf("Exiting...\n");
				send(*socket, &sendThis, 1, 0);
				return 0;
			default:
				// Handle invalid keys
				//printf("Invalid key: %c\n", ch);
				break;
			}
			send(*socket, &sendThis, 1, 0);
		}
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
	
	for (int i = 0; i < MAX_PLAYERS; i++) {
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
						args->drawArgs->map[current->y][current->x] = ' ';
						current->x += current->direction.difX;
						current->y += current->direction.difY;

						// Check for food collision
						for (int j = 0; j < MAX_PLAYERS; j++) {
							if (current->x == args->food[j]->x && current->y == args->food[j]->y) {
								Position* newFoodPos = findEmptySpace(args);
								if (newFoodPos != NULL) {
									args->food[j]->x = newFoodPos->x;
									args->food[j]->y = newFoodPos->y;
									//args->food[j]->y += 1;
									args->drawArgs->map[args->food[j]->y][args->food[j]->x] = args->food[j]->segChar;
									free(newFoodPos);
								}
								Segment* next = (Segment*)malloc(sizeof(Segment));
								next->x = lastOriginalX;
								next->y = lastOriginalY;
								next->segChar = '0' + snakeLength;
								next->direction = DIRS[STOP];
								next->isAlive = TRUE;
								next->next = NULL;
								last->next = next;
							}
						}

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

						args->drawArgs->map[current->y][current->x] = current->segChar;
					}
					else {
						args->drawArgs->map[current->y][current->x] = ' ';
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
		if (args->drawArgs->map[randomHeight][randomWide] == ' ') {
			
			Position* pos = (Position*)malloc(sizeof(Position));
			pos->x = randomWide;
			pos->y = randomHeight;
			return pos;
		}
		attempts++;
	}
	return NULL;
}

