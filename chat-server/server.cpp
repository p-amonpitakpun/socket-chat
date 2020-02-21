#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <exception>
#include <thread>
#include <vector>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <conio.h>
#include <ctype.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512


char err[100];

class Server {
private:

	char PORT[10] = "8080";

	WSADATA wsaData;
	int iResult, iSendResult;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	// buffers
	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN] = "";
	int sendbuflen = DEFAULT_BUFLEN;
	char sendbuf[DEFAULT_BUFLEN] = "";

	std::vector<std::thread> clientThreads;

public:

	Server(const char* port) {

		strcpy_s(PORT, port);

		// Initialize Winsock
		printf(">> Initialize Winsock... ");
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			sprintf_s(err, "WSAStartup failed: %d\r\n", iResult);

			throw new std::exception(err);
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

			throw new std::exception(err);
		}
		printf("\t\tCOMPLETED\r\n");

		// Create a SOCKET
		printf(">> Create a SOCKET... ");
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			sprintf_s(err, "Error at socket(): %d\r\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();

			throw new std::exception(err);
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

			throw new std::exception(err);
		}
		printf("\t\t\tCOMPLETED\r\n");

		freeaddrinfo(result);
	}

	struct Client {
		size_t id;
		SOCKET socket;
	};
	typedef struct Client Client;
	std::vector<Client> clients;

	void handleClient(size_t id, SOCKET clientSocket) {
		printf(">> ---- handle new client (%d)\r\n", (int)id);
		int iResult;
		int recvbuflen = DEFAULT_BUFLEN;
		char recvbuf[DEFAULT_BUFLEN];
		do {
			iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				printf(">> client[%d] send %d Bytes\r\n", (int)id, iResult);

				// send to all other client
				for (Client client : clients) {
					if (client.id != id) {
						send(client.socket, recvbuf, recvbuflen, 0);
					}
				}
			}
		} while (iResult > 0);
		printf(">> client[%d] disconnected\r\n", (int)id);
	}

	int run() {

		// Listen on a SOCKET
		printf(">> Listen on a SOCKET... ");
		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			sprintf_s(err, "Listen failed with error: %d\r\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();

			throw new std::exception(err);
		}
		printf("\t\tCOMPLETED\r\n");

		for (size_t id = 0; id < SOMAXCONN; id++) {

			while (clientThreads.size() > SOMAXCONN);

			try {
				printf(">> wait for client to connect\r\n");
				SOCKET newClientSocket = accept(ListenSocket, NULL, NULL);
				if (newClientSocket == INVALID_SOCKET) {
					printf(">> accept failed: %d\r\n", WSAGetLastError());
					continue;
				}
				printf(">> new client(%zu) connected.\r\n", id);

				clientThreads.push_back(std::thread(&Server::handleClient, this, id, newClientSocket));
			}
			catch (std::exception & e) {
				printf(e.what());
				printf("\r\n");
			}
			
			printf(">> number of clients = %d / %d\r\n", (int)clientThreads.size(), (int)SOMAXCONN);
		}

		for (size_t i = 0; i < clientThreads.size(); i++) {
			clientThreads[i].join();
		}

		printf(">> Waiting for CLIENT... \r\n");
		ClientSocket = accept(ListenSocket, NULL, NULL);
		printf(">> CLIENT accepted\r\n");

		// shutdown
		printf(">> Shutdown the SOCKET... ");
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			sprintf_s(err, "shutdown failed: %d\r\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();

			throw new std::exception(err);
		}
		printf("\t\tCOMPLETED\r\n");

		// cleanup
		closesocket(ClientSocket);
		WSACleanup();

		return 0;
	}
};


int main()
{
	printf("/* --- Socket Chat Server --- */\r\n");

	char port[10] = "8080";

	try {
		Server server(port);
		server.run();
	}
	catch (std::exception& e) {
		printf(e.what());

		printf("press any key to exit...");
		_getch();
		return 1;
	}

	printf("press any key to exit...");
	_getch();
	return 0;
}