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

#define MAX_PLAYERS 2

typedef struct {
	int width;
	int height;
	char** map;
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

typedef struct {
	int x;
	int y;
	char segChar;
	Direction direction;
	struct Segment* next;
	BOOLEAN isAlive;
} Segment;

typedef struct {
	Segment* heads[MAX_PLAYERS];
	Segment* food[MAX_PLAYERS];
	DrawArgs* drawArgs;
	SRWLOCK tickLock;
} GameInfo;

typedef struct tickInfo {
	GameInfo* gameInfo;
	int clientSocket;
} TickInfo;

typedef struct ServerInfo {
	GameInfo* gameInfo;
	HANDLE tickEvent;
	SOCKET clientSocket;
	int playerID;
	_Bool isConnected;
	SRWLOCK mapLock;
} ServerInfo;

SNAKE_API int draw(void* arg);
SNAKE_API int inputHandler(void* arg);
SNAKE_API void updateSnake(void* arg);
SNAKE_API Position* findEmptySpace(void* arg);