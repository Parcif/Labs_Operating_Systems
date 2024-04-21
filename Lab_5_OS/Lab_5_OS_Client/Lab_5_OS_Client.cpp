#include<iostream> 
#include<string> 
#include<vector> 
#include<winsock2.h> 
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable: 4996)//для inet_addr 
using namespace std;

// проверка на то, отправилось и считалось ли  
bool printCausedBy(int Result, const char* nameOfOper)
{
	if (!Result)
	{
		cout << "Connection closed.\n";
		return false;
	}
	else if (Result < 0)
	{
		cout << nameOfOper;
		cerr << "failed:\n", WSAGetLastError();
		return false;
	}
	return true;
}


int main()
{
	//Загрузка библиотеки  
	WSAData wsaData; //создаем структуру для загрузки 
	//запрашиваемая версия библиотеки winsock 
	WORD DLLVersion = MAKEWORD(2, 1);

	//проверка подключения библиотеки 
	if (WSAStartup(DLLVersion, &wsaData) != 0) 
	{
		cerr << "Error: failed to link library.\n";
		return 1;
	}

	//Заполняем информацию об адресе сокета 
	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //адрес
	addr.sin_port = htons(1111); //порт 
	addr.sin_family = AF_INET; //семейство протоколов 

	//сокет для прослушвания порта 
	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);

	//проверка на подключение к серверу 
	if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr))) 
	{
		cout << "Unable to connect to server!\n";
		return 1;
	}

	//открываем семафор 
	HANDLE hSemaphore = OpenSemaphore(SYNCHRONIZE, FALSE, L"semaphore");
	if (hSemaphore == NULL) {
		cerr << "Error open semaphore.\n";
		return GetLastError();
	}

	WaitForSingleObject(hSemaphore, INFINITE);

	string expression;
	while (expression != "end")
	{
		cout << "Enter expression or <end>: ";
		getline(cin, expression);

		char buffer[1000];
		printCausedBy(send(Connection, expression.c_str(), expression.size() + 1, NULL), "Send");

		if (expression != "end")
		{
			printCausedBy(recv(Connection, buffer, sizeof(buffer), NULL), "Recv");
			cout << "Result: " << buffer << "\n\n";
		}
	}

	cout << "\nBye!";
	CloseHandle(hSemaphore);
	if (closesocket(Connection) == SOCKET_ERROR)
		cerr << "Failed to terminate connection.\n Error code: " << WSAGetLastError();
	WSACleanup();
	return 0;
}