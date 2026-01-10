#include <stdio.h>
#include <string.h>
#include <crtdbg.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Hadik.h"
#pragma comment(lib, "Ws2_32.lib")

#define SNAKE_PORT 12367
#define BUFFER_SIZE 4096
#define TICK_INTERVAL_MS 500

void spawnSnake(GameInfo* gameIn, int playerIndex) {
	if (gameIn != NULL) {
		Segment* snakeHead = (Segment*)malloc(sizeof(Segment));
		Position* playerPos = findEmptySpace(gameIn);
		if (playerPos != NULL) {
			snakeHead->x = playerPos->x;
			snakeHead->y = playerPos->y;
			free(playerPos);
		}
		else {
			snakeHead->x = 5 + playerIndex;
			snakeHead->y = 5;
		}
		snakeHead->segChar = 'A' + playerIndex;
		snakeHead->isAlive = TRUE;
		snakeHead->next = NULL;
		snakeHead->direction = DIRS[STOP];
		gameIn->heads[playerIndex] = snakeHead;
		gameIn->drawArgs->map[gameIn->heads[playerIndex]->y * gameIn->drawArgs->width + gameIn->heads[playerIndex]->x] =
			gameIn->heads[playerIndex]->segChar;
	}
}

void spawnFood(GameInfo* gameIn, int foodIndex) {
	if (gameIn != NULL) {
		Segment* foodSeg = (Segment*)malloc(sizeof(Segment));
		Position* foodPos = findEmptySpace(gameIn);
		if (foodPos != NULL) {
			foodSeg->x = foodPos->x;
			foodSeg->y = foodPos->y;
			free(foodPos);
		}
		else {
			foodSeg->x = 10 + foodIndex;
			foodSeg->y = 10;
		}
		foodSeg->segChar = '@';
		foodSeg->isAlive = TRUE;
		foodSeg->next = NULL;
		gameIn->food[foodIndex] = foodSeg;
		gameIn->drawArgs->map[gameIn->food[foodIndex]->y * gameIn->drawArgs->width + gameIn->food[foodIndex]->x] = gameIn->food[foodIndex]->segChar;
	}
}

int serverGameInfoInitializer(ServerInfo* info) {
	if (info == NULL) {
		return -1;
	}
	HANDLE dEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	DrawArgs* argum = info->gameInfo->drawArgs;
	InputInfo* inputs = info->gameInfo->inputInfo;

	if (inputs->newGameHeight > MAX_GAME_HEIGHT || inputs->newGameHeight < 0) {
		inputs->newGameHeight = DEFAULT_GAME_HEIGHT;
	}
	if (inputs->newGameWidth > MAX_GAME_WIDTH || inputs->newGameWidth < 0) {
		inputs->newGameWidth = DEFAULT_GAME_WIDTH;
	}
	if (inputs->newGamePlayerCount > MAX_PLAYERS || inputs->newGamePlayerCount < 0) {
		inputs->newGamePlayerCount = DEFAULT_PLAYER_COUNT;
	}
	if (inputs->newGameTimeLimit > MAX_GAME_TIME || inputs->newGameTimeLimit < 0) {
		inputs->newGameTimeLimit = DEFAULT_GAME_TIME;
	}

	if (!info->gameInfo->isRunning) {
		argum = (DrawArgs*)malloc(sizeof(DrawArgs));
		argum->height = info->gameInfo->inputInfo->newGameHeight;
		argum->width = info->gameInfo->inputInfo->newGameWidth;
		argum->dEvent = dEvent;

		char* map = (char*)malloc(argum->height * argum->width * sizeof(char*));
		argum->map = map;
		/*
		for (int i = 0; i < argum->height; i++) {
			map[i] = (char*)malloc(sizeof(char) * argum->width);
		}
		*/
		for (int i = 0; i < argum->height; i++) {
			for (int j = 0; j < argum->width; j++) {
				if (i == 0 || i == argum->height - 1) {
					map[i * argum->width + j] = '-';
				}
				else if (j == 0 || j == argum->width - 1) {
					map[i * argum->width + j] = '|';
				}
				else {
					map[i * argum->width + j] = ' ';
				}
			}
		}
	}
	else {
		argum->height = info->gameInfo->inputInfo->newGameHeight;
		argum->width = info->gameInfo->inputInfo->newGameWidth;

		char* map = (char*)realloc(argum->map, argum->height * argum->width * sizeof(char*));
		argum->map = map;

		/*
		for (int i = 0; i < argum->height; i++) {
			map[i] = (char*)realloc(argum->map[i], sizeof(char) * argum->width);
		}
		*/
		for (int i = 0; i < argum->height; i++) {
			for (int j = 0; j < argum->width; j++) {
				if (i == 0 || i == argum->height - 1) {
					map[i * argum->width + j] = '-';
				}
				else if (j == 0 || j == argum->width - 1) {
					map[i * argum->width + j] = '|';
				}
				else {
					map[i * argum->width + j] = ' ';
				}
			}
		}
	}
	GameInfo* gameIn = info->gameInfo;
	gameIn->drawArgs = argum;

	for (int i = 0; i < info->gameInfo->inputInfo->newGamePlayerCount; i++) {
		spawnSnake(gameIn, i);
	}
	for (int i = 0; i < info->gameInfo->inputInfo->newGamePlayerCount; i++) {
		spawnFood(gameIn, i);
	}

	if (1) {
		for (int i = 0; i < argum->height * argum->width / 10; i++) {
			Position* pos = findEmptySpace(gameIn);
			if (pos != NULL) {
				argum->map[pos->y * argum->width + pos->x] = 'X';
			}
		}
	}
	info->gameInfo->isRunning = TRUE;
	info->gameInfo = gameIn;
	
	return 0;
}

// Thread function to handle each client
DWORD WINAPI ClientSender(void* arg) {
	ServerInfo* info = (ServerInfo*)arg;
	SOCKET clientSocket = info->clientSocket;
	char buffer[BUFFER_SIZE];
	printf("Sender thread created for client %d.\n", info->playerID);
	while (1) {
		WaitForSingleObject(info->tickEvent, INFINITE);
		AcquireSRWLockShared(&info->mapLock);
		int expectedSize = info->gameInfo->drawArgs->height * info->gameInfo->drawArgs->width;
		memset(buffer, 0, BUFFER_SIZE);
		if (info->gameInfo != NULL && info->gameInfo->isRunning) {
			/*
			for (int i = 0; i < info->gameInfo->drawArgs->height; i++) {
				for (int j = 0; j < info->gameInfo->drawArgs->width; j++) {
					buffer[i * (info->gameInfo->drawArgs->width) + j] = info->gameInfo->drawArgs->map[i * info->gameInfo->drawArgs->width + j];
				}
			}
			*/
			memcpy(buffer, info->gameInfo->drawArgs->map, expectedSize);
		}
		else {
			memset(buffer, ' ', expectedSize);
		}
		ReleaseSRWLockShared(&info->mapLock);
		if (info->needToQuit) {
			printf("Sender thread %d terminated due to quitting.\n", info->playerID);
			break;
		}
		if (info->gameInfo->drawArgs != NULL) {
			send(clientSocket, buffer, expectedSize, 0);
		}
		else {
			send(clientSocket, buffer, 1, 0);
		}
	}

	return 0;
}

DWORD WINAPI ClientReceiver(void* arg) {
	ServerInfo* info = (ServerInfo*)arg;
	SOCKET clientSocket = info->clientSocket;
	char buffer[BUFFER_SIZE];
	printf("Receiver thread created for client %d.\n", info->playerID);
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Connection closed by client %d or error occurred.\n", info->playerID);
			info->isConnected = FALSE;
			break;
		}
		printf("Received from client: %s\n", buffer);
		// Process received data (e.g., update snake direction)
		AcquireSRWLockExclusive(&info->mapLock);
		_Bool newStatus = TRUE;
		Segment* head = NULL;
		if (info->gameInfo != NULL && info->gameInfo->isRunning) {
			head = info->gameInfo->heads[info->playerID];
		}
		Direction newDir;
		if (head != NULL) {
			newDir = head->direction;
		}
		else {
			newDir = DIRS[STOP];
		}
		switch (buffer[0]) {
		case 'w':
			newDir = DIRS[UP];
			break;
		case 'a':
			newDir = DIRS[LEFT];
			break;
		case 's':
			newDir = DIRS[DOWN];
			break;
		case 'd':
			newDir = DIRS[RIGHT];
			break;
		case 'q':
			newStatus = FALSE;
			info->needToQuit = TRUE;
			break;
		case 'p':
			newDir = DIRS[STOP];
			break;
		case 'n':
			bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
			InputInfo* received = (InputInfo*)buffer;
			if (bytesRead <= 0) {
				printf("Connection closed by client %d or error occurred.\n", info->playerID);
				info->isConnected = FALSE;
				break;
			}
			info->gameInfo->inputInfo->newGameWidth = received->newGameWidth;
			info->gameInfo->inputInfo->newGameHeight = received->newGameHeight;
			info->gameInfo->inputInfo->newGamePlayerCount = received->newGamePlayerCount;
			info->gameInfo->inputInfo->newGameTimeLimit = received->newGameTimeLimit;
			serverGameInfoInitializer(info);
			break;
		case '0':
			printf("Reallocation error had by client %d.\n", info->playerID);
			break;
		case 'r':
			if (head == NULL) {
				spawnSnake(info->gameInfo, info->playerID);
			}
			break;
		}
			
		if (head != NULL) {
			Segment* neck = head->next;
			if (neck != NULL && head->x + newDir.difX == neck->x && head->y + newDir.difY == neck->y) {
				// Ignore reverse direction
			}
			else {
				head->direction = newDir;
			}
			head->isAlive = newStatus;
		}
		ReleaseSRWLockExclusive(&info->mapLock);
		if (info->needToQuit) {
			printf("Connection quit by client %d.\n", info->playerID);
			info->isConnected = FALSE;
			break;
		}
	}
	closesocket(info->clientSocket);
	return 0;

}

DWORD WINAPI TickHandler(void* arg) {
	ServerInfo* tick = (ServerInfo*)arg;
	while (1) {
		Sleep(TICK_INTERVAL_MS); // Tick every second
		if (!tick->serverClosing) {
			SetEvent(tick->tickEvent);
			AcquireSRWLockExclusive(&tick->mapLock);
			updateSnake(tick->gameInfo);
			ReleaseSRWLockExclusive(&tick->mapLock);
		}
		else {
			break;
		}
		Sleep(25);
		ResetEvent(tick->tickEvent);
	}
	return 0;
}

ServerInfo* findEmptyServerInfoSlot(ServerInfo* infos) {
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (!infos[i].isConnected) {
			return &infos[i];
		}
	}
	return NULL;
}

int main() {
	{
		srand(time(NULL));

		WSADATA wsaData;
		int iResult;

		// Initialize Winsock version 2.2
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}

		int serverSocket;

		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket < 0) {
			printf("Socket creation failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		printf("Socket created successfully.\n");

		int opt = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		struct sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
		serverAddress.sin_port = htons(SNAKE_PORT);

		if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
			perror("Bind failed.\n");
			closesocket(serverSocket);
			return 2;
		}
		printf("Socket bound to port: %d\n", SNAKE_PORT);

		if (listen(serverSocket, 5) < 0) {
			perror("Listening failed.\n");
			closesocket(serverSocket);
			return 3;
		}
		printf("Server listening.\n");

		int clientSocket = 0;
		struct sockaddr_in clientAddress;
		socklen_t clienLength;
		clienLength = sizeof(clientAddress);
		/*
		clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clienLength);
		if (clientSocket < 0) {
			perror("Accept failed.\n");
			closesocket(serverSocket);
			return 4;
		}
		//printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
		*/


		HANDLE tickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ServerInfo serverInfo[MAX_PLAYERS];
		//serverGameInfoInitializer(&serverInfo[0]);
		InitializeSRWLock(&serverInfo[0].mapLock);
		serverInfo[0].gameInfo = (GameInfo*)malloc(sizeof(GameInfo));
		serverInfo[0].gameInfo->isRunning = FALSE;
		serverInfo[0].gameInfo->inputInfo = (InputInfo*)malloc(sizeof(InputInfo));
		serverGameInfoInitializer(&serverInfo[0]);
		for (size_t i = 0; i < MAX_PLAYERS; i++)
		{
			serverInfo[i].gameInfo = serverInfo[0].gameInfo;
			serverInfo[i].mapLock = serverInfo[0].mapLock;
			serverInfo[i].tickEvent = tickEvent;
			serverInfo[i].playerID = i;
			serverInfo[i].isConnected = FALSE;
			serverInfo[i].needToQuit = FALSE;
			serverInfo[i].serverClosing = FALSE;
		}

		HANDLE tThread = CreateThread(NULL, 0, TickHandler, &serverInfo[0], 0, NULL);
		if (tThread == NULL) {
			printf("Could not create server tick thread\n", GetLastError());
			closesocket(clientSocket);
			return 4;
		}
		else {
			CloseHandle(tThread); // Close the thread handle as we don't need it
		}

		while (1) {
			_Bool dbgAccept = FALSE;
			clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clienLength);
			ServerInfo* emptySlot = findEmptyServerInfoSlot(serverInfo);
			if (emptySlot == NULL) {
				printf("No available slots for new clients.\n");
				closesocket(clientSocket);
			}
			else {
				emptySlot->clientSocket = clientSocket;
				emptySlot->isConnected = TRUE;
				emptySlot->needToQuit = FALSE;
				if (clientSocket != INVALID_SOCKET) {
					printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
					/*
					if (emptySlot->gameInfo->heads[emptySlot->playerID] == NULL) {
						spawnSnake(emptySlot->gameInfo, emptySlot->playerID);
					}
					*/
					// Create a thread for this specific client
					HANDLE sThread = CreateThread(NULL, 0, ClientSender, emptySlot, 0, NULL);
					HANDLE rThread = CreateThread(NULL, 0, ClientReceiver, emptySlot, 0, NULL);
					if (sThread == NULL) {
						printf("Could not create sender thread for client: %d\n", GetLastError());
						closesocket(clientSocket);
						return 5;
					}
					else {
						CloseHandle(sThread); // Close the thread handle as we don't need it
					}
					if (rThread == NULL) {
						printf("Could not create receiver thread for client: %d\n", GetLastError());
						closesocket(clientSocket);
						return 6;
					}
					else {
						CloseHandle(rThread); // Close the thread handle as we don't need it
					}
					serverInfo[0].serverClosing = FALSE;
					dbgAccept = FALSE;
				}
			}
			if (dbgAccept) {
				break;
			}
		}

		for (int i = 0; i < serverInfo[0].gameInfo->drawArgs->height; i++) {
			free(serverInfo[0].gameInfo->drawArgs->map[i]);
		}
		free(serverInfo[0].gameInfo->drawArgs->map);
		free(serverInfo[0].gameInfo->drawArgs);
		for (int i = 0; i < MAX_PLAYERS; i++) {
			Segment* current = serverInfo[0].gameInfo->heads[i];
			if (current != NULL) {
				deSnake(current);
			}
			current = serverInfo[0].gameInfo->food[i];
			while (current != NULL) {
				Segment* temp = current;
				current = current->next;
				free(temp);
			}

		}
		free(serverInfo[0].gameInfo);
		
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		_CrtDumpMemoryLeaks();
	}
	return 0;
}
