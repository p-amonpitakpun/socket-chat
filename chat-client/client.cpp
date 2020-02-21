#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <exception>
#include <thread>
#include <shared_mutex>;
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

class Client {
private:
	char IP[20] = "127.0.0.1";
	char PORT[10] = "8080";

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];
	int sendbuflen = DEFAULT_BUFLEN;
	char sendbuf[DEFAULT_BUFLEN] = "Hi";

	WSADATA wsaData;
	int iResult, iSendResult;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	SOCKET ConnectSocket = INVALID_SOCKET;

	int inputbuflen = DEFAULT_BUFLEN;
	char inputbuf[DEFAULT_BUFLEN];
	char err[100];

	char prefix = '/';

	std::thread recvThread;
	std::thread sendThread;

	int isRunning = 1;

	std::shared_mutex sharedMutex;

public:
	Client(const char* ip, const char* port) {

		strcpy_s(IP, ip);
		strcpy_s(PORT, port);

		// Initialize Winsock
		printf(">> Initialize Winsock... ");
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			sprintf_s(err, "getaddrinfo failed with error: %d\r\n", iResult);

			throw new std::exception(err);
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

		iResult = getaddrinfo("127.0.0.1", port, &hints, &result);
		if (iResult != 0) {
			sprintf_s(err, "getaddrinfo failed: %d\r\n", iResult);
			WSACleanup();

			throw new std::exception(err);
		}
		printf("\t\tCOMPLETED\r\n");

	}

	void stopServer() {

		// shutdown
		printf(">> Shutdown the SOCKET... ");
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			sprintf_s(err, "shutdown failed: %d\r\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();

			throw new std::exception(err);
		}
		printf("\t\tCOMPLETED\r\n");

		// cleanup
		closesocket(ConnectSocket);
		WSACleanup();
		return;
	}

	void sendChat() {
		int iResult;
		do {
			//std::unique_lock<std::shared_mutex> lock(sharedMutex);

			/*----LOCK----*/

			printf("ME >> ");
			std::cin >> inputbuf;

			if (inputbuf[0] == prefix) {
				if (strcmp(inputbuf, "/exit") == 0) {
					isRunning = 0;
					stopServer();
					return;
				}
			}
			else {
				send(ConnectSocket, inputbuf, strlen(inputbuf), 0);
			}
			//lock.unlock();

			/*----UNLOCK----*/

		} while (isRunning);
	}

	void recvChat() {
		int iResult;
		int recvbuflen = DEFAULT_BUFLEN;
		char recvbuf[DEFAULT_BUFLEN] = "";
		do {
			memset(recvbuf, 0, sizeof(recvbuf));
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				//std::unique_lock<std::shared_mutex> lock(sharedMutex);

				/*----LOCK----*/

				printf("\r");
				printf(recvbuf);
				printf("ME >> ");

				/*----UNLOCK----*/
				//lock.unlock();
			}
			else if (iResult == SOCKET_ERROR) {
				printf("\r\n>> DISCONNECTED!\r\n");
				break;
			}
		} while (isRunning);
	}

	void run() {

		// Attempt to connect to an address
		printf(">> Attempt to connect... \r\n");
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			printf(">> ---- Create a SOCKET...");
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (ConnectSocket == INVALID_SOCKET) {
				sprintf_s(err, "socket failed with error: %d\r\n", WSAGetLastError());
				WSACleanup();

				throw new std::exception(err);
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

			throw new std::exception(err);
		}

		try {
			setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, "50", 2);
		}
		catch (std::exception & e) {
			printf(">> set socket option error\r\n");
		}

		printf("\r\n-------- LOBBY --------\r\n");

		recvThread = std::thread(&Client::recvChat, this);
		sendThread = std::thread(&Client::sendChat, this);

		recvThread.join();
		sendThread.join();

		stopServer();
	}
};

int main()
{
	printf("/* --- Socket Chat Client --- */\r\n");

	char ip[20];
	char port[10];
	/*
	printf(">> IP ADDRESS : ");
	std::cin >> ip;
	printf(">> PORT       : ");
	std::cin >> port;
	*/
	try {
		//Client client(ip, port);
		Client client("127.0.0.1", "8080");
		client.run();
	}
	catch (std::exception & e) {
		printf(e.what());

		printf("press any key to exit...");
		_getch();
		return 1;
	}

	printf("press any key to exit...");
	_getch();
	return 0;
}