#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <windows.h>
#include <conio.h>

#ifdef _WIN32
#define SNAKE_API __declspec(dllexport)
#else
#define SNAKE_API
#endif

#define MAX_PLAYERS 4

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
	RIGHT
};

const Direction DIRS[] = {
	[UP] = {0, -1},  // Up
	[DOWN] = {0, 1},   // Down
	[LEFT] = {-1, 0},  // Left
	[RIGHT] = {1, 0}    // Right
};

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
	DrawArgs* drawArgs;
} GameInfo;

typedef struct tickInfo {
	GameInfo* gameInfo;
	int clientSocket;
} TickInfo;

SNAKE_API int draw(void* arg);
SNAKE_API int inputHandler(void* arg);
void updateSnake(Segment* head, int width, int height);