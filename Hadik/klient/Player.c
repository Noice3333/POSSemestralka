#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define SNAKE_PORT 12367
#define BUFFER_SIZE 1024

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

	char buffer[BUFFER_SIZE];
	while (1) {
		fflush(stdout);

		if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
			printf("Error reading input.\n");
			break;
		}

		send(clientSocket, buffer, strlen(buffer), 0);
		
		if (strncmp(buffer, "exit", 4) == 0) {
			printf("Exiting client.\n");
			break;
		}

		memset(buffer, 0, BUFFER_SIZE);
		int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Connection closed by server or error occurred.\n");
			break;
		}

		printf("Server: %s", buffer);
	}

	closesocket(clientSocket);
	printf("Client closed.\n");
	WSACleanup();
	system("pause");
	return 0;
}