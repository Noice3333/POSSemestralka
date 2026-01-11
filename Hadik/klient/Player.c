#include "Player.h"

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
				AcquireSRWLockExclusive(tick->gameInfo->tickLock);
				tick->gameInfo->inputInfo->continueGame = FALSE;
				ReleaseSRWLockExclusive(tick->gameInfo->tickLock);
				SetEvent(tick->gameInfo->drawArgs->dEvent);
				return 1;
			}
			totalRead += bytesRead;
		}
		MapPacket* mapInfo = (MapPacket*)buffer;
		MapPacket localMapInfo = *mapInfo;
		mapInfo = NULL;
		memset(buffer, 0, BUFFER_SIZE);
		totalRead = 0;
		char* localBuffer = malloc(localMapInfo.mapSize);
		if (localBuffer) {
			while (totalRead < localMapInfo.mapSize) {
				int bytesRead = recv(tick->clientSocket, localBuffer + totalRead, localMapInfo.mapSize - totalRead, 0);
				if (bytesRead < 0) {
					printf("Connection closed by server or error occurred.\n");
					tick->gameInfo->inputInfo->continueGame = FALSE;
					ReleaseSRWLockExclusive(tick->gameInfo->tickLock);
					SetEvent(tick->gameInfo->drawArgs->dEvent);
					return 1;
				}
				totalRead += bytesRead;
			}
		}
		AcquireSRWLockExclusive(tick->gameInfo->tickLock);
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
		tick->gameInfo->gameTime = localMapInfo.gameTime;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			tick->gameInfo->playerScores[i] = localMapInfo.playerScores[i];
		}
		memcpy(tick->gameInfo->drawArgs->map, localBuffer, localMapInfo.mapSize);
		ReleaseSRWLockExclusive(tick->gameInfo->tickLock);
		free(localBuffer);
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

	char* map = (char*)malloc(argum->height * argum->width * sizeof(char));
	argum->map = map;

	TickInfo tickIn;
	tickIn.gameInfo = (GameInfo*)malloc(sizeof(GameInfo));
	tickIn.clientSocket = clientSocket;
	tickIn.gameInfo->drawArgs = argum;
	SRWLOCK tickLock;
	InitializeSRWLock(&tickLock);
	tickIn.gameInfo->tickLock = &tickLock;

	for (int i = 0; i < MAX_PLAYERS; i++) {
		tickIn.gameInfo->playerScores[i] = -1;
	}
	tickIn.gameInfo->gameTime = 0;

	InputInfo inputIn;
	inputIn.clientSocket = clientSocket;
	inputIn.mode = MODE_MENU;
	inputIn.continueGame = TRUE;

	tickIn.gameInfo->inputInfo = &inputIn;
	

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

		BOOL optVal = TRUE;
		setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, sizeof(BOOL));

		printf("Connected to server successfully.\n");

		consoleDrawGameWindow(clientSocket);

		closesocket(clientSocket);
		printf("Client closed.\n");
		WSACleanup();
		system("pause");
		_CrtDumpMemoryLeaks();
	}
	return 0;
}

