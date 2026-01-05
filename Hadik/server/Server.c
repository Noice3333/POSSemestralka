#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define SNAKE_PORT 12367
#define BUFFER_SIZE 1024

// Thread function to handle each client
DWORD WINAPI ClientHandler(LPVOID lpParam) {
	SOCKET clientSocket = (SOCKET)lpParam;
	char buffer[BUFFER_SIZE];
	printf("Thread created for client.\n");
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		/*
		int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Client disconnected.\n");
			break;
		}

		printf("Incoming message: %s", buffer);

		if (strncmp(buffer, "quit", 4) == 0) {
			printf("Ending connection.\n");
			break;
		}
		*/
		Sleep(200);
		buffer[0] = 't';
		printf("Sending message: %s\n", buffer);
		send(clientSocket, buffer, strlen(buffer), 0);
	}
	closesocket(clientSocket);
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

	int serverSocket;
	
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) {
		printf("Socket creation failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	printf("Socket created successfully.\n");

	int opt = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

	int clientSocket;
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
	printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
	*/
	char buffer[BUFFER_SIZE];
	
	while (1) {
		
		memset(buffer, 0, BUFFER_SIZE);
		/*
		int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
		if (bytesRead <= 0) {
			printf("Client disconnected.\n");
			break;
		}
		
		printf("Incoming message: %s\n", buffer);
		
		if (strncmp(buffer, "quit", 4) == 0) {
			printf("Ending connection.\n");
			break;
		}

		send(clientSocket, buffer, strlen(buffer), 0);
		*/
		clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clienLength);
		if (clientSocket != INVALID_SOCKET) {
			printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
			// Create a thread for this specific client
			HANDLE hThread = CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
			if (hThread == NULL) {
				printf("Could not create thread for client: %d\n", GetLastError());
				closesocket(clientSocket);
			}
			else {
				CloseHandle(hThread); // Close the thread handle as we don't need it
			}
		}
	}
	closesocket(serverSocket);
	WSACleanup();
	system("pause");
	return 0;
}
