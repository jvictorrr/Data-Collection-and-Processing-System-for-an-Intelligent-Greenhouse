#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Defines
#define COMMAND_BUFF_SIZE 50
//Sockets Defines
#define DEFAULT_PORT "27069"
#define DEFAULT_BUFLEN 512

//Includes
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <string>
#include <chrono>
#include <ctime>
#include <windows.h>
#include <atomic>
#include <mutex>
//Sockets includes
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
using namespace std;
//Sockets includes
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mutex>

mutex m;


// Global Variables *******************************************************
HANDLE hStdin;
DWORD fdwSaveOldMode;
bool flagExecuteCommand = false;
string delimiter = ",";

//Setpoint
atomic<int> buff_T = 20; 
atomic<int> buff_L = 20; 




//Controlador + Sensor
atomic<double> tempinput = 15;
atomic<double> luminput = 15;

atomic<double> TempSensor = 5; //SENSOR DO CONJUGADO MECÂNICO CM; //ENVIA PRO SERVIDOR
atomic<double> TempLum = 5; //SENSOR DO CONJUGADO MECÂNICO CM; //ENVIA PRO SERVIDOR



//atomic<bool> ledAlerta = false;	//ALERTA: CM ACIMA DE 50. MUITA CARGA NO MOTOR!!! //RECEBE DO SERVIDOR
atomic<bool> TempAlerta = false; //Alerta Temperatura excessiva 
atomic<bool> LumAlerta = false; //Alerta Temperatura excessiva 


//SOCKETS GLOBAL
WSAData wsaData;
int iResult;
// Functions Prototypes ***************************************************
void ErrorExit(LPSTR);
void appSetup(void);
void appExit(void);
int AlertLum = 0;
int AlertTemp = 0;
// Threads ****************************************************************
void readCommand(char* command)
{
	DWORD cNumRead, fdwMode, i;
	INPUT_RECORD irInBuf[128];
	KEY_EVENT_RECORD ker;
	char keyRead;
	std::string concatKeys;

	while (true)
	{
		// Read Keys ====================================

		// Wait for the events.
		if (!ReadConsoleInput(
			hStdin,      // input buffer handle
			irInBuf,     // buffer to read into
			128,         // size of read buffer
			&cNumRead)) // number of records read
			ErrorExit((LPSTR)"ReadConsoleInput");

		// Dispatch the events to the appropriate handler.
		for (i = 0; i < cNumRead; i++)
		{
			switch (irInBuf[i].EventType)
			{
			case KEY_EVENT: // keyboard input
				// KeyEventProc(irInBuf[i].Event.KeyEvent);
				ker = irInBuf[i].Event.KeyEvent;
				keyRead = ker.uChar.AsciiChar;

				if (ker.bKeyDown)
				{
					std::cout << keyRead;
					switch (ker.wVirtualKeyCode)
					{
					case VK_RETURN:
						std::cout << std::endl;

						// Return the Command 
						memcpy(command, concatKeys.c_str(), concatKeys.size());
s

						concatKeys = "";
						flagExecuteCommand = true;

						break;

					case VK_BACK:
						std::cout << " \b";
						concatKeys.pop_back();
						break;

					case VK_SHIFT:
					case VK_CAPITAL:
						break;

					default:
						concatKeys += keyRead;
						break;
					}
				}
				break;

			default:
				//ErrorExit( (LPSTR) "Unknown event type");
				break;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::cout << "Exit readCommand!" << std::endl;
}

void executeCommand(char* command, int commandSize)
{
	while (true)
	{
		if (flagExecuteCommand)
		{
			char* ptr = strstr(command, "set");
			char* ptrVar;
			char varName[32] = "";
			int varValue = 0;

			if (ptr != NULL)
			{
				ptr += 4;
				ptrVar = strstr(ptr, "=");

				memcpy(varName, ptr, (ptrVar - ptr));

				if (ptrVar != NULL)
				{
					ptrVar += 1;
					varValue = std::atoi(ptrVar);
				}

				if (!strcmp(varName, "TempSensor"))
				{
					std::cout << varName << "=" << varValue << std::endl;
					TempSensor.store(varValue);
				}
				else if (!strcmp(varName, "TempLum"))
				{
					std::cout << varName << "=" << varValue << std::endl;
					TempLum.store(varValue);
				}
			}
			else if (!strcmp(command, "exit"))
			{
				ExitProcess(0);
			}
			else if (!strcmp(command, "checks"))
			{
				std::cout << "Sensors ------- " << std::endl;
				std::cout << "\tTempSensor: " << TempSensor.load() << std::endl;
				std::cout << "\tTempLum: " << TempLum.load() << std::endl;
				std::cout << std::endl;

			}
			else if (!strcmp(command, "clear"))
			{
				system("cls");
			}
			else if (!strcmp(command, "help"))
			{
				std::cout << "Defined commands:" << std::endl;
				std::cout << "\texit - exit the program." << std::endl;
				std::cout << "\tstatus - show selected variables. You need to change this function to show the desired variables." << std::endl;
				std::cout << "\tclear - clear the screen." << std::endl;
				std::cout << "\tset - set a new value for a defined variable. Syntax: set <VARIABLE_NAME>=<VALUE>" << std::endl;
			}
			else if (!strcmp(command, "status"))
			{
				std::cout << " ------- Setpoints ------- " << std::endl;
				std::cout << "\tbuff_T: " << buff_T.load() << std::endl;
				std::cout << "\tbuff_L: " << buff_L.load() << std::endl;
				std::cout << std::endl;

				std::cout << " ------- Sensors ------- " << std::endl;
				std::cout << "\tTempSensor: " << TempSensor.load() << std::endl;
				std::cout << "\tLumSensor: " << TempLum.load() << std::endl;
				std::cout << "\tAlert Temp: " << TempAlerta.load() << std::endl;
				std::cout << "\tAlert Lum: " << LumAlerta.load() << std::endl;

				std::cout << std::endl;

			}
			else
			{
				std::cout << "Command not found!" << std::endl;
			}


			std::cout << ">>>";

			memset(command, 0, commandSize);
			flagExecuteCommand = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::cout << "Exit executeCommand!" << std::endl;
}


void MOD_T() {
	while (true) {
		buff_T= TempSensor.load();
		this_thread::sleep_for(chrono::milliseconds(600));
	}
}

void MOD_L() {

	while (true) {
		buff_L = TempLum.load();
		this_thread::sleep_for(chrono::milliseconds(1800));
	}

}


//RECEBER VARIÁVEIS DE: Temperatura e Luminosidade  - DO SERVIDOR
void CommunicationThread(SOCKET client) {

	double input = 0, tempSensor = 0, output = 0, tempLum = 0;
	int  Alert = 0;
	bool x = true, y = true;

	if (iResult == SOCKET_ERROR) {
		cout << "Non-Blocking Mode Error" << "\n";
	}
	while (x || y) {
		int recvbuflen = DEFAULT_BUFLEN;
		char recvbuf[DEFAULT_BUFLEN];

		input = 0;

		tempSensor = TempSensor.load();
		tempLum = TempLum.load();
		AlertLum = 0;
		AlertTemp = 0;

		//ENVIAR VALORES PARA O SERVIDOR;
		string sendValues;
		sendValues = to_string(input) + "," + to_string(tempSensor) + "," + to_string(tempLum);
		const char* sendbuf = sendValues.c_str();
		iResult = send(client, sendbuf, (int)strlen(sendbuf), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			x = false;
		}
		//RECEBER VALORES DO SERVIDOR
		iResult = recv(client, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			string messageReceived(recvbuf);
			AlertLum = stod(messageReceived.substr(0, messageReceived.find(delimiter)));
			AlertTemp = stoi(messageReceived.substr(messageReceived.find(delimiter) + delimiter.length(), messageReceived.length()));

			//DEFINIR VARIÁVEIS COMPARTILHADAS ENTRE THREADS
			

			if (AlertTemp == 0) {
				TempAlerta.store(false);
			}
			else {
				TempAlerta.store(true);

			}
			if (AlertLum == 0)
				LumAlerta.store(false);
			else
				LumAlerta.store(true);
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			y = false;
		}
		this_thread::sleep_for(chrono::microseconds(200)); //TEMPO DE AMOSTRAGEM
	}
	cout << "Servidor Desconectou" << "\n";
	closesocket(client);
	WSACleanup();
}

void AlarmTask1() {
	
	while (true){
		int aux = TempAlerta.load();
		if (aux == 1)
			cout << "AlarmTask1" << "\n";
		this_thread::sleep_for(chrono::milliseconds(1600));
	}
	

}
void AlarmTask2() {
	while (true) {
		if (LumAlerta.load() == 1)
			cout << "AlarmTask2" << "\n";
		this_thread::sleep_for(chrono::milliseconds(1600));
	}

}
// Main + CRIAR SOCKET ********************************************************************
int __cdecl main(int argc, char** argv)
{
	//SOCKET CLIENT
		//INICIALIZANDO SOCKET
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	//CRIANDO SOCKET - Cliente
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	SOCKET ConnectSocket = INVALID_SOCKET;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ptr = result;
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		// CONECTAR SOCKET
			//Conectar ao servidor
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//RESTO DA MAIN
	char command[COMMAND_BUFF_SIZE] = "";

	appSetup();
	//THREADS DA APLICAÇÃO

	thread readCommandThread(readCommand, command);
	thread execCommandThread(executeCommand, command, COMMAND_BUFF_SIZE);
	thread modTthread(MOD_T);
	thread modLthread(MOD_L);
	thread tCommunicationThread(CommunicationThread, ConnectSocket);
	thread AlarmTask1Thread(AlarmTask1);
	thread AlarmTask2Thread(AlarmTask2);

	readCommandThread.join();
	execCommandThread.join();

	//JOIN DAS THREADS DA APLICAÇÃO
	modTthread.join();
	modLthread.join();
	tCommunicationThread.join();
	AlarmTask1Thread.join();
	AlarmTask2Thread.join();

	appExit();
}


// Funções ***************************************************************

void ErrorExit(LPSTR lpszMessage)
{
	fprintf(stderr, "%s\n", lpszMessage);

	// Restore input mode on exit.
	SetConsoleMode(hStdin, fdwSaveOldMode);

	ExitProcess(0);
}

void appSetup(void)
{
	// Get the standard input handle.
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
	{
		ErrorExit((LPSTR)"GetStdHandle");
	}

	// Save the current input mode, to be restored on exit.
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
	{
		ErrorExit((LPSTR)"GetConsoleMode");
	}

	std::cout << ">>> ";
}

void appExit(void)

{
	// Restore input mode on exit.
	SetConsoleMode(hStdin, fdwSaveOldMode);

	ExitProcess(0);
}