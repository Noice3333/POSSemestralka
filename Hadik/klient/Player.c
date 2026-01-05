#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include "Hadik.h"
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define SNAKE_PORT 12367
#define BUFFER_SIZE 1024

int gameTick(void* arg) {
	TickInfo* tick = (TickInfo*)arg;
	char buffer[BUFFER_SIZE];

	while (1) {
		fflush(stdout);
		/*
		if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
			printf("Error reading input.\n");
			break;
		}

		send(clientSocket, buffer, strlen(buffer), 0);

		if (strncmp(buffer, "exit", 4) == 0) {
			printf("Exiting client.\n");
			break;
		}
		*/
		memset(buffer, 0, BUFFER_SIZE);
		int bytesRead = recv(tick->clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Connection closed by server or error occurred.\n");
			break;
		}
		printf("%s", buffer);
		SetEvent(tick->gameInfo->drawArgs->dEvent);
	}
}

int consoleDrawGameWindow(int clientSocket) {

	HANDLE dEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	system("cls");
	DrawArgs argum;
	argum.height = 20;
	argum.width = 30;
	argum.dEvent = dEvent;

	char** map = (char**)malloc(argum.height * sizeof(char*));
	argum.map = map;

	
	for (int i = 0; i < argum.height; i++) {
		map[i] = (char*)malloc(sizeof(char) * argum.width);
	}

	GameInfo gamIn;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		Segment* snakeHead = (Segment*)malloc(sizeof(Segment));
		snakeHead->x = 5 + i;
		snakeHead->y = 5;
		snakeHead->segChar = 'A' + i;
		snakeHead->direction = DIRS[DOWN];
		snakeHead->isAlive = TRUE;
		snakeHead->next = NULL;
		gamIn.heads[i] = snakeHead;
	}
	gamIn.heads[1]->direction = DIRS[LEFT];
	gamIn.heads[2]->direction = DIRS[UP];
	gamIn.heads[3]->direction = DIRS[RIGHT];
	gamIn.drawArgs = &argum;

	TickInfo tickIn;
	tickIn.gameInfo = &gamIn;
	tickIn.clientSocket = clientSocket;
	
	for (int i = 0; i < argum.height; i++) {
		for (int j = 0; j < argum.width; j++) {
			if (i == 0 || i == argum.height - 1) {
				map[i][j] = '-';
			}
			else if (j == 0 || j == argum.width - 1) {
				map[i][j] = '|';
			}
			else {
				map[i][j] = ' ';
			}
		}
	}

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (gamIn.heads[i]->isAlive) {
			map[gamIn.heads[i]->y][gamIn.heads[i]->x] = gamIn.heads[i]->segChar;
		}
	}
	

	thrd_t drawThread;
	thrd_t inputThread;
	thrd_t tickThread;
	thrd_create(&drawThread, draw, &gamIn);
	thrd_create(&inputThread, inputHandler, &gamIn);
	thrd_create(&tickThread, gameTick, &tickIn);

	thrd_join(drawThread, NULL);
	thrd_join(inputThread, NULL);
	thrd_join(tickThread, NULL);

	for (int i = 0; i < argum.height; i++) {
		free(map[i]);
	}
	free(map);
	system("pause");
	return 0;
}

int main() {
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
	return 0;
}

