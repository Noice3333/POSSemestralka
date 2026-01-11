#include "client.h"

int inputHandler(void* arg) {
	GameInfo* input = (GameInfo*)arg;
	int* socket = &input->inputInfo->clientSocket;
	char sendThis = ' ';
	char buffer[10];
	while (1) {
		if (_kbhit()) {
			int ch = _getch();
			AcquireSRWLockExclusive(input->tickLock);
			int* currentMode = &input->inputInfo->mode;
			ReleaseSRWLockExclusive(input->tickLock);
			if (*currentMode == MODE_MENU) {
				switch (ch) {
				case 'n': case 'N':
					AcquireSRWLockExclusive(input->tickLock);
					if (input->isRunning == 1) {
						sendThis = 'n';
						ReleaseSRWLockExclusive(input->tickLock);
						break;
					}
					system("cls");
					printf("Starting new game... (press enter)\n");
					clear_buffer();
					printf("Enter width (min 10, max %d): ", MAX_GAME_WIDTH);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialWidth = DEFAULT_GAME_WIDTH;
					potentialWidth = strtol(buffer, NULL, 10);
					if (potentialWidth > MAX_GAME_WIDTH || potentialWidth < 10) {
						printf("Invalid input for width, defaulting to %d\n", DEFAULT_GAME_WIDTH);
						potentialWidth = DEFAULT_GAME_WIDTH;
					}

					printf("Enter height (min 10, max %d): ", MAX_GAME_HEIGHT);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialHeight = DEFAULT_GAME_HEIGHT;
					potentialHeight = strtol(buffer, NULL, 10);
					if (potentialHeight > MAX_GAME_HEIGHT || potentialHeight < 10) {
						printf("Invalid input for height, defaulting to %d\n", DEFAULT_GAME_HEIGHT);
						potentialHeight = DEFAULT_GAME_HEIGHT;
					}

					printf("Enter number of players (min 1, max %d): ", MAX_PLAYERS);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialNumOfPlayers = DEFAULT_PLAYER_COUNT;
					potentialNumOfPlayers = strtol(buffer, NULL, 10);
					if (potentialNumOfPlayers > MAX_PLAYERS || potentialNumOfPlayers < 1) {
						printf("Invalid input for number of players, defaulting to %d\n", DEFAULT_PLAYER_COUNT);
						potentialNumOfPlayers = DEFAULT_PLAYER_COUNT;
					}

					printf("Enter time limit: (0 = no limit, max %d): ", MAX_GAME_TIME);
					fgets(buffer, 10, stdin);
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}
					int potentialTimeLimit = DEFAULT_GAME_TIME;
					potentialTimeLimit = strtol(buffer, NULL, 10);
					if (potentialTimeLimit > MAX_GAME_TIME || potentialTimeLimit < 0) {
						printf("Invalid input for time limit, defaulting to %d\n", DEFAULT_GAME_TIME);
						potentialTimeLimit = DEFAULT_GAME_TIME;
					}

					printf("Do you want obstacles? Type in only 'y' if you do: ");
					int got = 0;
					got = fgetc(stdin);
					_Bool obstacles = DEFAULT_OBSTACLE_STATE;
					if (got != 'y') {
						printf("Defaulting to no obstacles.\n");
						obstacles = DEFAULT_OBSTACLE_STATE;
					}
					else {
						obstacles = TRUE;
					}
					if (strchr(buffer, '\n') == NULL) {
						clear_buffer();
					}

					input->inputInfo->newGameHeight = potentialHeight;
					input->inputInfo->newGameWidth = potentialWidth;
					input->inputInfo->newGamePlayerCount = potentialNumOfPlayers;
					input->inputInfo->newGameTimeLimit = potentialTimeLimit;
					input->inputInfo->obstacles = obstacles;
					input->drawArgs->height = potentialHeight;
					input->drawArgs->width = potentialWidth;

					system("pause");
					input->inputInfo->mode = MODE_PLAY;
					ReleaseSRWLockExclusive(input->tickLock);
					system("cls");
					sendThis = 'n';
					break;
				case 'j': case 'J':
					AcquireSRWLockExclusive(input->tickLock);
					*currentMode = MODE_PLAY;
					ReleaseSRWLockExclusive(input->tickLock);
					sendThis = 'j';
					break;
				case 'c': case 'C':
					sendThis = 'c';
					AcquireSRWLockExclusive(input->tickLock);
					*currentMode = MODE_PLAY;
					ReleaseSRWLockExclusive(input->tickLock);
					break;
				case 'e': case 'E':
					sendThis = 'e';
					AcquireSRWLockExclusive(input->tickLock);
					input->inputInfo->continueGame = FALSE;
					ReleaseSRWLockExclusive(input->tickLock);
					send(*socket, &sendThis, 1, 0);
					return 0;
				}
			}
			else if (*currentMode == MODE_PLAY) {
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
				case 'm': case 'M':
					AcquireSRWLockExclusive(input->tickLock);
					*currentMode = MODE_MENU;
					ReleaseSRWLockExclusive(input->tickLock);
					sendThis = 'm';
					break;
				case 'r': case 'R':
					sendThis = 'r';
					break;
				default:
					break;
				}
			}
			send(*socket, &sendThis, 1, 0);
			switch (sendThis) {
			case 'n':
				send(*socket, (char*)input->inputInfo, sizeof(InputInfo), 0);
				break;
			case 'j':
				break;
			}
		}
		AcquireSRWLockExclusive(input->tickLock);
		if (!input->inputInfo->continueGame) {
			ReleaseSRWLockExclusive(input->tickLock);
			break;
		}
		ReleaseSRWLockExclusive(input->tickLock);
	}
	return 0;
}


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
		tick->gameInfo->inputInfo->permissionToConnect = localMapInfo.permissionToConnect;
		tick->gameInfo->isRunning = localMapInfo.isRunning;
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

	
	free(tickIn.gameInfo->drawArgs->map);
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

