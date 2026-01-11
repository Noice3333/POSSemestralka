#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")




#ifdef _WIN32
#define SNAKE_API __declspec(dllexport)
#else
#define SNAKE_API
#endif

#define MAX_PLAYERS 4
#define MAX_GAME_WIDTH 70
#define MAX_GAME_HEIGHT 30
#define MAX_GAME_TIME 180
#define DEFAULT_GAME_WIDTH 30
#define DEFAULT_GAME_HEIGHT 20
#define DEFAULT_GAME_TIME 0
#define DEFAULT_PLAYER_COUNT 1
#define DEFAULT_OBSTACLE_STATE 0

typedef struct {
	int width;
	int height;
	char* map;
	HANDLE dEvent;
} DrawArgs;

typedef struct {
	int difX;
	int difY;
} Direction;

enum DirectionEnum {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	STOP
};

enum ControlMode {
	MODE_PLAY,
	MODE_MENU
};

enum MenuItem {
	MENU_NEW_GAME,
	MENU_JOIN_GAME,
	MENU_CONTINUE,
	MENU_EXIT
};

const Direction DIRS[] = {
	[UP] = {0, -1},  // Up
	[DOWN] = {0, 1},   // Down
	[LEFT] = {-1, 0},  // Left
	[RIGHT] = {1, 0},    // Right
	[STOP] = {0, 0}    // Stop
};

typedef struct Position {
	int x;
	int y;
} Position;

typedef struct Segment {
	struct Segment* next;
	int x;
	int y;
	Direction direction;
	_Bool isAlive;
	_Bool isPaused;
	char segChar;
} Segment;

typedef struct InputInfo {
	int newGameWidth;
	int newGameHeight;
	int newGamePlayerCount;
	int newGameTimeLimit;
	int clientSocket;
	int mode;
	_Bool obstacles;
	_Bool continueGame;
	_Bool permissionToConnect;
} InputInfo;

typedef struct MapPacket {
	int width;
	int height;
	int mapSize;
	int playerScores[MAX_PLAYERS];
	int gameTime;
	_Bool permissionToConnect;

} MapPacket;

typedef struct {
	Segment* heads[MAX_PLAYERS];
	Segment* food[MAX_PLAYERS];
	DrawArgs* drawArgs;
	SRWLOCK* tickLock;
	InputInfo* inputInfo;
	int playerScores[MAX_PLAYERS];
	int elapsedTimeMS;
	int gameTime;
	_Bool isRunning;
} GameInfo;

typedef struct TickInfo {
	GameInfo* gameInfo;
	int clientSocket;
} TickInfo;



typedef struct ServerInfo {
	GameInfo* gameInfo;
	HANDLE tickEvent;
	SOCKET clientSocket;
	int playerID;
	_Bool isConnected;
	_Bool needToQuit;
	_Bool serverClosing;
	SRWLOCK* mapLock;
} ServerInfo;

SNAKE_API int draw(void* arg);
SNAKE_API int inputHandler(void* arg);
SNAKE_API void updateSnake(void* arg);
SNAKE_API Position* findEmptySpace(void* arg);