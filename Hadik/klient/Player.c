#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include "Hadik.h"


#define SNAKE_PORT 12367
#define BUFFER_SIZE 4096

int gameTick(void* arg) {
	TickInfo* tick = (TickInfo*)arg;
	char buffer[BUFFER_SIZE];

	while (1) {
		if (!tick->gameInfo->inputInfo->continueGame) {
			SetEvent(tick->gameInfo->drawArgs->dEvent);
			break;
		}
		memset(buffer, 0, BUFFER_SIZE);
		int totalRead = 0;

		while (totalRead < sizeof(MapPacket)) {
			int bytesRead = recv(tick->clientSocket, buffer + totalRead, sizeof(MapPacket) - totalRead, 0);
			if (bytesRead <= 0) {
				printf("Connection closed by server or error occurred.\n");
				return 1;
			}
			totalRead += bytesRead;
		}
		MapPacket* mapInfo = (MapPacket*)buffer;
		MapPacket localMapInfo = *mapInfo;
		mapInfo = NULL;
		memset(buffer, 0, BUFFER_SIZE);
		totalRead = 0;
		
		AcquireSRWLockExclusive(&tick->gameInfo->tickLock);
		if (localMapInfo.height * localMapInfo.width == localMapInfo.mapSize &&
			localMapInfo.height >= 10 && localMapInfo.width >= 10 &&
			localMapInfo.height <= MAX_GAME_HEIGHT &&
			localMapInfo.width <= MAX_GAME_WIDTH) {
			if (localMapInfo.height != tick->gameInfo->drawArgs->height ||
				localMapInfo.width != tick->gameInfo->drawArgs->width) {
				char* new = (char*)realloc(tick->gameInfo->drawArgs->map, localMapInfo.mapSize);
				tick->gameInfo->drawArgs->map = new;
				tick->gameInfo->drawArgs->height = localMapInfo.height;
				tick->gameInfo->drawArgs->width = localMapInfo.width;
			}
		}
		if (localMapInfo.mapSize > 0) {
			while (totalRead < localMapInfo.mapSize) {
				int bytesRead = recv(tick->clientSocket, tick->gameInfo->drawArgs->map + totalRead, localMapInfo.mapSize - totalRead, 0);
				if (bytesRead < 0) {
					printf("Connection closed by server or error occurred.\n");
					tick->gameInfo->inputInfo->continueGame = FALSE;
					ReleaseSRWLockExclusive(&tick->gameInfo->tickLock);
					SetEvent(tick->gameInfo->drawArgs->dEvent);
					return 1;
				}
				totalRead += bytesRead;
			}
		}
		ReleaseSRWLockExclusive(&tick->gameInfo->tickLock);
		SetEvent(tick->gameInfo->drawArgs->dEvent);
	}
	return 0;
}

int consoleDrawGameWindow(int clientSocket) {
	
	HANDLE dEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	DrawArgs* argum = (DrawArgs*)malloc(sizeof(DrawArgs));
	argum->height = DEFAULT_GAME_HEIGHT;
	argum->width = DEFAULT_GAME_WIDTH;
	argum->dEvent = dEvent;

	char* map = (char*)malloc(argum->height * argum->width * sizeof(char*));
	argum->map = map;

	/*
	for (int i = 0; i < argum->height; i++) {
		map[i] = (char*)malloc(sizeof(char) * argum->width);
	}
	*/
	TickInfo tickIn;
	tickIn.gameInfo = (GameInfo*)malloc(sizeof(GameInfo));
	tickIn.clientSocket = clientSocket;
	tickIn.gameInfo->drawArgs = argum;
	InitializeSRWLock(&tickIn.gameInfo->tickLock);

	InputInfo inputIn;
	inputIn.clientSocket = clientSocket;
	inputIn.mode = MODE_MENU;
	inputIn.continueGame = TRUE;

	tickIn.gameInfo->inputInfo = &inputIn;
	/*
	for (int i = 0; i < argum->height; i++) {
		for (int j = 0; j < argum->width; j++) {
			if (i == 0 || i == argum->height - 1) {
				map[i][j] = '-';
			}
			else if (j == 0 || j == argum->width - 1) {
				map[i][j] = '|';
			}
			else {
				map[i][j] = ' ';
			}
		}
	}
	*/

	thrd_t drawThread;
	thrd_t inputThread;
	thrd_t tickThread;
	thrd_create(&drawThread, draw, tickIn.gameInfo);
	thrd_create(&inputThread, inputHandler, tickIn.gameInfo);
	thrd_create(&tickThread, gameTick, &tickIn);

	thrd_join(drawThread, NULL);
	thrd_join(inputThread, NULL);
	thrd_join(tickThread, NULL);

	
	free(map);
	free(tickIn.gameInfo->drawArgs);
	free(tickIn.gameInfo);

	return 0;
}

int main() {
	{
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock version 2.2
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}

		int clientSocket;
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket == INVALID_SOCKET) {
			printf("Socket creation failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		printf("Socket created successfully.\n");

		struct sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(SNAKE_PORT);
		if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
			printf("Invalid address/ Address not supported \n");
			closesocket(clientSocket);
			return 2;
		}

		if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
			printf("Connection to server failed with error: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			return 3;
		}

		printf("Connected to server successfully.\n");

		// Game logic starts here


		consoleDrawGameWindow(clientSocket);



		closesocket(clientSocket);
		printf("Client closed.\n");
		WSACleanup();
		system("pause");
		_CrtDumpMemoryLeaks();
	}
	return 0;
}

