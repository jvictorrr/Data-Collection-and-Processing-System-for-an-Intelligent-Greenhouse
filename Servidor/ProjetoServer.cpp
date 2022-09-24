//SERVIDOR DA APLICAÇÃO
#undef UNICODE

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib,"Ws2_32.lib")
//INCLUDES
#include <windows.h>
#include<WinSock2.h>
#include <WS2tcpip.h>
#include<iphlpapi.h>
#include <string>
#include <iostream>
#include <thread>
using namespace std;

//DEFINES
#define DEFAULT_PORT "27069" //PORTA DO SOCKET SERVIDOR ESCOLHIDA.
#define DEFAULT_BUFLEN 512
#define LUM_BUFF 1024
//VARIÁVEIS GLOBAIS
WSADATA wsaData;
int iResult;
int iSendResult;
string delimiter = ",";



//VARIÁVEIS DE ENTRADA, SAÍDA E SENSORES
double input = 0;
double input_lum = 0;
double TempSensor = 0;
double TempLum = 0;

double sensorAlertLum = 0;

double output = 12;
int AlertTemp = 0;
int AlertLum = 0;



void handleClients(SOCKET Client) {
	char recvbuf[LUM_BUFF];
	int recvbuflen = LUM_BUFF;
	int len = 0;
	bool x = true;
	bool y = true;

	while (x || y) {
		//RECEBER ENTRADAS
		iResult = recv(Client, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			len = iResult;
			string messageReceived(recvbuf), helperString;
			input = stod(messageReceived.substr(0, messageReceived.find(delimiter)));
			helperString = messageReceived.erase(0, messageReceived.find(delimiter) + delimiter.length());
			TempSensor= stod(helperString.substr(0, helperString.find(delimiter)));
			TempLum = stod(helperString.substr(helperString.find(delimiter) + delimiter.length(), helperString.length()));
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			x = false;
		}
		cout << "TempSensor: " << TempSensor << "\n";
		cout << "TempLum: " << TempLum << "\n";
		if (TempSensor > 28)
			AlertTemp = 1;
		if (TempSensor <= 28)
			AlertTemp = 0;
		if (TempLum > 37)
			AlertLum = 1;
		if (TempLum <= 37)
			AlertLum = 0;
		//ENVIAR SAÍDAS
		string outString = to_string(AlertLum) + "," + to_string(AlertTemp);
		const char* outputSend = outString.c_str();
		iSendResult = send(Client, outputSend, (int)strlen(outputSend), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			y = false;
		}
	}
	printf("CLIENTE ENCERROU A SESSAO.\n");
}

//ESCUTAR O SOCKET E CRIAR THREAD DE CONVERSA COM CLIENTE(S)
void handleListens(SOCKET ListenSocket) {
	bool x = true;
	while (x) {
		SOCKET ClientSocket = INVALID_SOCKET;
		//LISTENING TO A SOCKET
		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n", WSAGetLastError());
			x = false;
		}
		else {
			printf("ESCUTAR UM SOCKET\n");
			//ACEITAR CONEXÃO DO SOCKET
			ClientSocket = INVALID_SOCKET;
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("accept failed: %d\n", WSAGetLastError());
			}
			printf("ACEITAR CONEXAO DO SOCKET\n");
			thread Clients(handleClients, ClientSocket);
			Clients.join();
		}
	}
	closesocket(ListenSocket);
	WSACleanup();
}


int __cdecl main(int argc, char** argv) {


	//INICIALIZAR WINSOCK
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {	//Check For Errors
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	} 
	printf("INICIALIZAR WINSOCK\n");
	//CRIAR SOCKET
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	printf("CRIAR SOCKET\n");
	//BIND DO SOCKET
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	printf("BIND DO SOCKET\n");
	//ESCUTAR O SOCKET
	thread ListenAndAccept(handleListens, ListenSocket);
	ListenAndAccept.join();


}