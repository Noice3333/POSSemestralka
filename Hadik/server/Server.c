#include "server.h"

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
		snakeHead->isPaused = FALSE;
		snakeHead->next = NULL;
		snakeHead->direction = DIRS[STOP];
		snakeHead->pauseMS = WAIT_UNPAUSE_MS;
		gameIn->heads[playerIndex] = snakeHead;
		gameIn->drawArgs->map[gameIn->heads[playerIndex]->y * gameIn->drawArgs->width + gameIn->heads[playerIndex]->x] =
			gameIn->heads[playerIndex]->segChar;
		gameIn->playerScores[playerIndex] = 1;
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
		argum->width = info->gameInfo->inputInfo->newGameWidth + 1;
		argum->dEvent = dEvent;

		char* map = (char*)malloc(argum->height * argum->width * sizeof(char));
		argum->map = map;
		
	}
	else {
		argum->height = info->gameInfo->inputInfo->newGameHeight;
		argum->width = info->gameInfo->inputInfo->newGameWidth;

		char* map = (char*)realloc(argum->map, argum->height * argum->width * sizeof(char));
		argum->map = map;
	}

	for (int i = 0; i < argum->height; i++) {
		for (int j = 0; j < argum->width; j++) {
			if (i == 0 || i == argum->height - 1) {
				argum->map[i * argum->width + j] = '-';
			}
			else if (j == 0 || j == argum->width - 1) {
				argum->map[i * argum->width + j] = '|';
			}
			else {
				argum->map[i * argum->width + j] = ' ';
			}
		}
	}

	GameInfo* gameIn = info->gameInfo;
	gameIn->drawArgs = argum;
	info->gameInfo->gameTime = 0;

	for (int i = 0; i < info->gameInfo->inputInfo->newGamePlayerCount; i++) {
		
	}
	spawnSnake(gameIn, info->playerID);
	for (int i = 0; i < info->gameInfo->inputInfo->newGamePlayerCount; i++) {
		spawnFood(gameIn, i);
	}

	if (info->gameInfo->inputInfo->obstacles) {
		for (int i = 0; i < argum->height * argum->width / 20; i++) {
			Position* pos = findEmptySpace(gameIn);
			if (pos != NULL) {
				argum->map[pos->y * argum->width + pos->x] = '=';
				free(pos);
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
	char buffer[BUFFER_SIZE];
	printf("Sender thread created for clients.\n");
	while (!info[0].serverClosing) {
		WaitForSingleObject(info[0].tickEvent, INFINITE);
		AcquireSRWLockShared(info[0].mapLock);
		MapPacket packet;
		packet.height = DEFAULT_GAME_HEIGHT;
		packet.mapSize = 0;
		packet.width = DEFAULT_GAME_WIDTH;
		packet.permissionToConnect = TRUE;
		packet.isRunning = info[0].gameInfo->isRunning;
		_Bool toPermit[MAX_PLAYERS];
		for (int i = 0; i < MAX_PLAYERS; i++) {
			packet.playerScores[i] = info[0].gameInfo->playerScores[i];
			if (info[0].gameInfo->heads[i] != NULL) {
				toPermit[i] = TRUE;
			}
			else {
				toPermit[i] = FALSE;
			}
		}
		packet.gameTime = info[0].gameInfo->gameTime;
		int expectedSize = 0;
		memset(buffer, 0, BUFFER_SIZE);
		if (info[0].gameInfo != NULL && info[0].gameInfo->isRunning) {
			expectedSize = info[0].gameInfo->drawArgs->height * info[0].gameInfo->drawArgs->width;
			packet.mapSize = expectedSize;
			packet.height = info[0].gameInfo->drawArgs->height;
			packet.width = info[0].gameInfo->drawArgs->width;
			memcpy(buffer, info[0].gameInfo->drawArgs->map, expectedSize);
		}
		ReleaseSRWLockShared(info[0].mapLock);
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (info[i].isConnected) {
				SOCKET clientSocket = info[i].clientSocket;
				packet.permissionToConnect = toPermit[i];
				int result = send(clientSocket, (void*)&packet, sizeof(MapPacket), 0);
				if (result <= 0) {
					info[i].isConnected = FALSE;
				}
				if (result > 0 && expectedSize > 0) {
					result = send(clientSocket, buffer, packet.mapSize, 0);
					if (result <= 0) {
						info[i].isConnected = FALSE;
					}
				}
			}
		}
	}
	return 0;
}

DWORD WINAPI ClientReceiver(void* arg) {
	srand(time(NULL));
	ServerInfo* info = (ServerInfo*)arg;
	SOCKET clientSocket = info->clientSocket;
	char buffer[BUFFER_SIZE];
	printf("Receiver thread created for client %d.\n", info->playerID);
	while (!info->serverClosing) {
		memset(buffer, 0, BUFFER_SIZE);
		int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Connection closed by client %d or error occurred.\n", info->playerID);
			AcquireSRWLockExclusive(info->mapLock);
			info->gameInfo->heads[info->playerID]->isAlive = FALSE;
			ReleaseSRWLockExclusive(info->mapLock);
			info->isConnected = FALSE;
			break;
		}
		printf("Received from client: %s\n", buffer);
		// Process received data (e.g., update snake direction)
		AcquireSRWLockExclusive(info->mapLock);
		_Bool newStatus = TRUE;
		Segment* head = NULL;
		if (info->gameInfo != NULL && info->gameInfo->isRunning) {
			head = info->gameInfo->heads[info->playerID];
		}
		Direction newDir;
		newDir = DIRS[STOP];
		if (head != NULL && info->gameInfo->isRunning) {
			newDir = head->direction;
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
			if (!info->gameInfo->isRunning) {
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
				info->gameInfo->inputInfo->obstacles = received->obstacles;
				serverGameInfoInitializer(info);
			}
			else {
				printf("Game is already running at this time.\n");
			}
			break;
		case '0':
			printf("Reallocation error had by client %d.\n", info->playerID);
			break;
		case 'j':
			int playerCount = 0;
			for (int i = 0; i < MAX_PLAYERS; i++) {
				Segment* head = info->gameInfo->heads[i];
				if (head != NULL) {
					playerCount++;
				}
			}
			if (playerCount < info->gameInfo->inputInfo->newGamePlayerCount) {
				if (head == NULL) {
					spawnSnake(info->gameInfo, info->playerID);
					for (int y = 0; y < MAX_PLAYERS; y++) {
						if (info->gameInfo->heads[y] != NULL) {
							info->gameInfo->heads[y]->pauseMS = 0;
						}
					}
				}
			}
			break;
		case 'e':
			info->needToQuit = TRUE;
			newStatus = FALSE;
			break;
		case 'm':
			if (info->gameInfo->heads[info->playerID] != NULL)
				info->gameInfo->heads[info->playerID]->isPaused = TRUE;
				info->gameInfo->heads[info->playerID]->pauseMS = 0;
			break;
		case 'c':
			if (info->gameInfo->heads[info->playerID] != NULL) {
				info->gameInfo->heads[info->playerID]->isPaused = FALSE;
				info->gameInfo->heads[info->playerID]->pauseMS = 0;
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
		ReleaseSRWLockExclusive(info->mapLock);
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
	srand(time(NULL));
	ServerInfo* tick = (ServerInfo*)arg;
	int noPlayersConnectedMS = 0;
	while (1) {
		Sleep(TICK_INTERVAL_MS); // Tick every second
		if (!tick[0].serverClosing) {
			SetEvent(tick[0].tickEvent);
			AcquireSRWLockExclusive(tick[0].mapLock);
			int connectedPlayers = 0;
			for (int i = 0; i < MAX_PLAYERS; i++) {
				ServerInfo* info = &tick[i];
				if (info->isConnected) {
					connectedPlayers++;
				}
			}
			if (connectedPlayers > 0) {
				noPlayersConnectedMS = 0;
				updateSnake(tick[0].gameInfo);
				if (tick[0].gameInfo->isRunning) {
					tick[0].gameInfo->gameTime += TICK_INTERVAL_MS;
					if (tick[0].gameInfo->inputInfo->newGameTimeLimit > 0 &&
						tick[0].gameInfo->gameTime / 1000 >= tick[0].gameInfo->inputInfo->newGameTimeLimit) {
						tick[0].gameInfo->isRunning = FALSE;
						printf("Time limit reached. Game over!\n");
					}
					else {
						int playerCount = 0;
						for (int i = 0; i < MAX_PLAYERS; i++) {
							if (tick[0].gameInfo->heads[i] != NULL) {
								playerCount++;
							}
						}
						if (playerCount > 0) {
							tick[0].gameInfo->elapsedTimeMS = 0;
						}
						else {
							tick[0].gameInfo->elapsedTimeMS += TICK_INTERVAL_MS;
						}
						if (tick[0].gameInfo->elapsedTimeMS >= 10000) {
							tick[0].gameInfo->isRunning = FALSE;
							printf("No players playing for 10 seconds. Stopping the game.\n");
						}
					}
				}
			}
			else {
				noPlayersConnectedMS += TICK_INTERVAL_MS;
			}
			if (noPlayersConnectedMS >= 10000) {
				printf("No players connected for 10 seconds. Stopping the server.\n");
				for (int i = 0; i < MAX_PLAYERS; i++) {
					closesocket(tick[i].clientSocket);
					tick[i].serverClosing = TRUE;
				}
				ResetEvent(tick[0].tickEvent);
				SetEvent(tick[0].tickEvent);
			}
			ReleaseSRWLockExclusive(tick[0].mapLock);
		}
		else {
			break;
		}
		Sleep(25);
		ResetEvent(tick[0].tickEvent);
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
		HANDLE tickEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ServerInfo serverInfo[MAX_PLAYERS];
		SRWLOCK maplock;
		InitializeSRWLock(&maplock);
		serverInfo[0].mapLock = &maplock;
		serverInfo[0].gameInfo = (GameInfo*)malloc(sizeof(GameInfo));
		serverInfo[0].gameInfo->isRunning = FALSE;
		serverInfo[0].gameInfo->inputInfo = (InputInfo*)malloc(sizeof(InputInfo));
		serverInfo[0].gameInfo->elapsedTimeMS = 0;
		serverInfo[0].gameInfo->drawArgs = NULL;
		serverInfo[0].gameInfo->gameTime = 0;
		for (size_t i = 0; i < MAX_PLAYERS; i++)
		{
			serverInfo[i].gameInfo = serverInfo[0].gameInfo;
			
			serverInfo[0].gameInfo->heads[i] = NULL;
			serverInfo[0].gameInfo->food[i] = NULL;

			serverInfo[i].mapLock = serverInfo[0].mapLock;
			serverInfo[i].tickEvent = tickEvent;
			serverInfo[i].playerID = i;
			serverInfo[i].isConnected = FALSE;
			serverInfo[i].needToQuit = FALSE;
			serverInfo[i].serverClosing = FALSE;
			
		}

		HANDLE tThread = CreateThread(NULL, 0, TickHandler, serverInfo, 0, NULL);
		if (tThread == NULL) {
			printf("Could not create server tick thread %d\n", GetLastError());
			closesocket(clientSocket);
			return 4;
		}
		else {
			CloseHandle(tThread); // Close the thread handle as we don't need it
		}
		HANDLE sThread = CreateThread(NULL, 0, ClientSender, serverInfo, 0, NULL);
		if (sThread == NULL) {
			printf("Could not create sender thread for clients.\n", GetLastError());
			return 5;
		}
		else {
			CloseHandle(sThread); // Close the thread handle as we don't need it
		}

		while (!serverInfo[0].serverClosing) {
			struct timeval timeout;
			timeout.tv_sec = 4; // Check every 4 seconds
			timeout.tv_usec = 0;

			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(serverSocket, &readfds);

			// 2. Wait for a connection OR timeout
			int activity = select(0, &readfds, NULL, NULL, &timeout);

			if (activity == SOCKET_ERROR) {
				printf("Select failed: %d\n", WSAGetLastError());
				break;
			}

			if (activity > 0) {
				clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clienLength);
				if (serverInfo[0].serverClosing) {
					break;
				}
				if (clientSocket != INVALID_SOCKET) {
					BOOL optVal = TRUE;
					int optLen = sizeof(BOOL);

					if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) == SOCKET_ERROR) {
						printf("Failed to set TCP_NODELAY for cleint. Error: %d\n", WSAGetLastError());
					}
					else {
						printf("TCP_NODELAY set for client.\n");
					}

					ServerInfo* emptySlot = findEmptyServerInfoSlot(serverInfo);
					if (emptySlot == NULL) {
						printf("No available slots for new clients.\n");
						closesocket(clientSocket);
					}
					else {
						emptySlot->clientSocket = clientSocket;
						emptySlot->isConnected = TRUE;
						emptySlot->needToQuit = FALSE;

						printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
						
						HANDLE rThread = CreateThread(NULL, 0, ClientReceiver, emptySlot, 0, NULL);
						if (rThread == NULL) {
							printf("Could not create receiver thread for client: %d\n", GetLastError());
							closesocket(clientSocket);
							return 6;
						}
						else {
							CloseHandle(rThread); // Close the thread handle as we don't need it
						}
					}
				}
			}
		}

		if (serverInfo[0].gameInfo->drawArgs != NULL) {
			free(serverInfo[0].gameInfo->drawArgs->map);
			free(serverInfo[0].gameInfo->drawArgs);
		}
		free(serverInfo[0].gameInfo->inputInfo);
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
		_CrtDumpMemoryLeaks();
	}
	return 0;
}
