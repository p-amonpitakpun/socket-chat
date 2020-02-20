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
#define PORT "8080"

int recvbuflen = DEFAULT_BUFLEN;
char recvbuf[DEFAULT_BUFLEN];
int sendbuflen = DEFAULT_BUFLEN;
char sendbuf[DEFAULT_BUFLEN] = "Hi";

WSADATA wsaData;
int iResult, iSendResult;
struct addrinfo* result = NULL, * ptr = NULL, hints;
SOCKET ConnectSocket = INVALID_SOCKET;

char err[100];

int startClient() {
	// Initialize Winsock
	printf(">> Initialize Winsock... ");
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		sprintf_s(err, "getaddrinfo failed with error: %d\r\n", iResult);

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

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

	// Attempt to connect to an address
	printf(">> Attempt to connect... \r\n");
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		printf(">> ---- Create a SOCKET...");
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			sprintf_s(err, "socket failed with error: %d\r\n", WSAGetLastError());
			WSACleanup();

			return 1;
		}
		printf("\t\tCOMPLETED\r\n");

		printf(">> ---- Connect to server... ");
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("\t\tFAILED!\r\n");
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;

			continue;
		}
		printf("\t\tCOMPLETED\r\n");
		break;
	}
	freeaddrinfo(result);
	printf(">> connected to server\r\n");

	if (ConnectSocket == INVALID_SOCKET) {
		printf("ERROR INVALID SOCKET\r\n");
		WSACleanup();

		return 1;
	}
	/*
	iResult = send(ConnectSocket, sendbuf, (int)sendbuflen, 0);
	if (iResult == SOCKET_ERROR) {
		sprintf_s(err, "send failed: %d\r\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();

		return 1;
	}

	printf("\r\n>> Byte Sent: %ld\n", iResult);
	printf(">> Sent: ");
	printf(sendbuf);
	printf("\r\n");
	*/


	do {
		iResult = recv(ConnectSocket, recvbuf, (int)recvbuflen, 0);
		if (iResult > 0) {
			printf(">> Byte received: %d\r\n", iResult);
			printf(">> Received: ");
			printf(recvbuf);
			printf("\r\n");
		}
		else if (iResult == 0) {
			printf("Connection closed\r\n");
		}
		else {
			printf("recv failed: %d\r\n", WSAGetLastError());
		}
	} while (iResult > 0);

	// shutdown
	printf(">> Shutdown the SOCKET... ");
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		sprintf_s(err, "shutdown failed: %d\r\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();

		return 1;
	}
	printf("\t\tCOMPLETED\r\n");

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}

int main()
{
	printf("/* --- Socket Chat Client --- */\r\n");

	if (startClient() != 0){
		printf(err);

		printf("press KEY to exit.");
		std::cin.sync();
		std::cin.get();

		return 1;
	}

	printf("client end\r\n");
	printf("press KEY to exit.");
	std::cin.sync();
	std::cin.get();
	return 0;
}