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

void updateSnake(void* arg) {
	GameInfo* args = (GameInfo*)arg;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (args->heads[i] != NULL && args->heads[i]->isAlive) {
			args->heads[i]->x += args->heads[i]->direction.difX;
			args->heads[i]->y += args->heads[i]->direction.difY;

			if (args->heads[i]->x == args->drawArgs->width - 1) {
				args->heads[i]->x = 1;
			}
			else if (args->heads[i]->x == 0) {
				args->heads[i]->x = args->drawArgs->width - 2;
			}
			if (args->heads[i]->y == args->drawArgs->height - 1) {
				args->heads[i]->y = 1;
			}
			else if (args->heads[i]->y == 0) {
				args->heads[i]->y = args->drawArgs->height - 2;
			}

		}
	}
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (args->heads[i]->isAlive) {
			// Draw at new position
			args->drawArgs->map[args->heads[i]->y][args->heads[i]->x] = args->heads[i]->segChar;
		}
	}
}