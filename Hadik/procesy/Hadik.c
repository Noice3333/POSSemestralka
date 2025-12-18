#include "Hadik.h"


typedef struct {
	int width;
	int height;
	char** map;
} DrawArgs;

int draw(void* arg) {
	DrawArgs* args = (DrawArgs*)arg;

	int counterX = 1;
	int counterY = 1;

	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);


	while (1) {
		COORD cursorPosition = { 0, 0 };
		args->map[counterY][counterX] = 'S';
		SetConsoleCursorPosition(consoleHandle, cursorPosition);
		for (int i = 0; i < args->height; i++) {
			for (int j = 0; j < args->width; j++) {
				printf("%c", args->map[i][j]);
				if (j == args->width - 1) {
					printf("\n");
				}
			}
		}
		args->map[counterY][counterX] = ' ';
		if (counterX + 1 < args->width - 1)
			counterX++;
		else
			counterX = 1;
		if (counterY + 1 < args->height - 1)
			counterY++;
		else
			counterY = 1;
		Sleep(0);
	}
	return 0;
}

int main() {
	
	system("cls");
	DrawArgs argum;
	argum.height = 20;
	argum.width = 50;
	
	char** map = (char**)malloc(argum.height*sizeof(char*));

	argum.map = map;
	for (int i = 0; i < argum.height; i++) {
		map[i] = (char*)malloc(sizeof(char) * argum.width);
	}

	for (int i = 0; i < argum.height; i++) {
		for (int j = 0; j < argum.width; j++) {
			if (i == 0 || i == argum.height - 1) {
				map[i][j] = 'o';
			}
			else if (j == 0 || j == argum.width - 1) {
				map[i][j] = '|';
			}
			else {
				map[i][j] = ' ';
			}
		}
	}
	


	thrd_t drawThread;
	thrd_create(&drawThread, draw, &argum);


	thrd_join(drawThread, NULL);
	
	for (int i = 0; i < argum.height; i++) {
		free(map[i]);
	}
	free(map);
	system("pause");
	return 0;
}