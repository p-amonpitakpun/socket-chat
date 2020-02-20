#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <exception>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

char PORT[10] = "8080";
int recvbuflen = DEFAULT_BUFLEN;
char recvbuf[DEFAULT_BUFLEN];

WSADATA wsaData;
int iResult, iSendResult;
struct addrinfo* result = NULL, * ptr = NULL, hints;
SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

char err[100];

int startServer() {
		
	// Initialize Winsock
	printf(">> Initialize Winsock... ");
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		sprintf_s(err, "WSAStartup failed: %d\r\n", iResult);

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	// Get address information
	printf(">> Get address information... ");
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo("127.0.0.1", PORT, &hints, &result);
	if (iResult != 0) {
		sprintf_s(err, "getaddrinfo failed: %d\r\n", iResult);
		WSACleanup();

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	// Create a SOCKET
	printf(">> Create a SOCKET... ");
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		sprintf_s(err, "Error at socket(): %d\r\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();

		return 1;
	}
	printf("\t\t\tCOMPLETED\r\n");

	// Bind the SOCKET
	printf(">> Bind the SOCKET... ");
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		sprintf_s(err, "bind failed with error: %d\r\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();

		return 1;
	}
	printf("\t\t\tCOMPLETED\r\n");

	freeaddrinfo(result);

	// Listen on a SOCKET
	printf(">> Listen on a SOCKET... ");
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		sprintf_s(err, "Listen failed with error: %d\r\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	printf(">> Waiting for CLIENT... \r\n");
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		sprintf_s(err, "accept failed: %d\r\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();

		return 1;
	}
	printf(">> CLIENT accepted\r\n");

	printf(">> Begin LOOP... \r\n");
	int i = 0;
	do {
		iSendResult = send(ClientSocket, "A", 1, 0);
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		printf("\r\n>> [%d]iResult: %d\r\n", i, iResult);
		if (iResult > 0) {
			printf("Bytes received: %d\r\n", iResult);

			// Echo
			iSendResult = send(ClientSocket, "A", 1, 0);
			if (iSendResult == SOCKET_ERROR) {
				sprintf_s(err, "send failed: %d\r\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();

				return 1;
			}
			printf(">> Byte Sent: %ld\n", iResult);
			printf("\r\n");
		}
		else if (iResult == 0) {
			printf("Connection closing...\r\n");
		}
		else {
			sprintf_s(err, "recv failed: %d\r\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();

			return 1;
		}
		i++;
		i %= 10;
	} while (iResult > 0);
	printf(">> End LOOP... \r\n");

	// shutdown
	printf(">> Shutdown the SOCKET... ");
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		sprintf_s(err, "shutdown failed: %d\r\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}

int main()
{
	printf("/* --- Socket Chat Server --- */\r\n");

	if(startServer() != 0) {
		printf(err);

		printf("press KEY to exit.");
		std::cin.sync();
		std::cin.get();

		return 1;
	}
	
	printf("server end\r\n");
	printf("press KEY to exit.");
	std::cin.sync();
	std::cin.get();
	return 0;
}