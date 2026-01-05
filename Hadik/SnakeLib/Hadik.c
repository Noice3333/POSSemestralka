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
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (args->heads[i]->isAlive) {
				// Update position
				updateSnake(args->heads[i], args->drawArgs->width, args->drawArgs->height);
				// Draw at new position
				args->drawArgs->map[args->heads[i]->y][args->heads[i]->x] = args->heads[i]->segChar;
			}
		}
		for (int i = 0; i < args->drawArgs->height; i++) {
			for (int j = 0; j < args->drawArgs->width; j++) {
				printf("%c", args->drawArgs->map[i][j]);
				if (j == args->drawArgs->width - 1) {
					printf("\n");
				}
			}
		}
	}
	return 0;
}

int inputHandler(void* arg) {
	GameInfo* args = (GameInfo*)arg;
	while (1) {
		if (_kbhit()) {
			Segment* player = args->heads[0];
			int ch = _getch();
			switch (ch) {
			case 'w': case 'W':
				player->direction = DIRS[UP];
				break;
			case 'a': case 'A':
				player->direction = DIRS[LEFT];
				break;
			case 's': case 'S':
				player->direction = DIRS[DOWN];
				break;
			case 'd': case 'D':
				player->direction = DIRS[RIGHT];
				break;
			case 'q': case 'Q':
				printf("Exiting...\n");
				return 0;
			default:
				// Handle invalid keys
				printf("Invalid key: %c\n", ch);
				break;
			}
		}
	}
	return 0;
}

void updateSnake(Segment* head, int width, int height) {
	if (head != NULL) {
		head->x += head->direction.difX;
		head->y += head->direction.difY;
		
		if (head->x == width - 1) {
			head->x = 1;
		}
		else if (head->x == 0) {
			head->x = width - 2;
		}
		if (head->y == height - 1) {
			head->y = 1;
		}
		else if (head->y == 0) {
			head->y = height - 2;
		}
		
	}
}