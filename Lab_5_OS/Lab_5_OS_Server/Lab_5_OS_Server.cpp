#include <iostream> 
#include <string> 
#include <vector> 
#include <stack>
#include<winsock2.h> 
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable: 4996) // для inet_addr 
using namespace std;

HANDLE hSemaphore;
struct ClientData 
{
	SOCKET socket;
	int client_number;
};

int read_int()
{
	int result;
	string input;
	bool incorrect = true;

	while (incorrect)
	{
		getline(cin, input);
		try
		{
			result = stoi(input);
			if (input.size() == to_string(result).size() && result > 0 && result < 11)
			{
				incorrect = false;
				return result;
			}
			else
				cout << "\nError! Try again: ";
		}
		catch (invalid_argument& e)
		{
			cout << "\nError! Try again: ";
		}
		catch (out_of_range& e)
		{
			cout << "\nError! Try again: ";
		}
	}
}

bool isValidExpression(string expression)
{
	if (expression.empty()) // Проверяем, что выражение не пустое
		return false;

	// Проверяем корректность символов
	for (char c : expression)
	{
		if (!isdigit(c) && c != '+' && c != '-' && c != '*' && c != '/' && c != '.')
		{
			return false;
		}
	}

	// Проверяем, что выражение не начинается с оператора
	if (expression[0] == '+' || expression[0] == '-' || expression[0] == '*' || expression[0] == '/')
		return false;

	// Проверяем, что выражение не заканчивается оператором
	if (expression.back() == '+' || expression.back() == '-' || expression.back() == '*' || expression.back() == '/')
		return false;

	// Проверяем, что два оператора не идут подряд
	for (size_t i = 0; i < expression.size() - 1; ++i)
	{
		if ((expression[i] == '+' || expression[i] == '-' || expression[i] == '*' || expression[i] == '/') &&
			(expression[i + 1] == '+' || expression[i + 1] == '-' || expression[i + 1] == '*' || expression[i + 1] == '/'))
			return false;
	}

	// Проверка деления на 0
	size_t pos = 0;
	while ((pos = expression.find('/', pos)) != std::string::npos)
	{
		if (pos < expression.size() - 1 && expression[pos + 1] == '0')
		{
			return false; // Деление на ноль
		}
		pos++;
	}

	return true;
}

// Функция для получения приоритета оператора
int getPriority(char op)
{
	if (op == '+' || op == '-')
		return 1;
	else if (op == '*' || op == '/')
		return 2;
	else
		return 0;
}

// Функция для вычисления результата
double applyOperator(double a, double b, char op)
{
	switch (op)
	{
	case '+': return a + b;
	case '-': return a - b;
	case '*': return a * b;
	case '/': return a / b;
	default: return 0;
	}
}

string formatResult(double value)
{
	string str = to_string(value);

	// Удаление лишних нулей и точки, если число целое
	str.erase(str.find_last_not_of('0') + 1, string::npos);
	str.erase(str.find_last_not_of('.') + 1, string::npos);

	return str;
}

bool calculateExpression(string& expression)
{
	// Проверяем корректность выражения перед вычислением
	if (!isValidExpression(expression))
		return false;

	stack<double> values;
	stack<char> operators;
	string number;

	for (char c : expression)
	{
		if (isdigit(c) || c == '.') // Если цифра или точка
		{
			number += c;
		}
		else if (c == '+' || c == '-' || c == '*' || c == '/') // Если оператор
		{
			values.push(stod(number)); // Помещаем число в стек значений
			number.clear();
			// Если символ - оператор, выполняем операции, пока оператор в стеке имеет больший или равный приоритет
			while (!operators.empty() && getPriority(operators.top()) >= getPriority(c))
			{
				double b = values.top(); values.pop(); // Присваиваем и удаляем верхний элемент стека
				double a = values.top(); values.pop();
				char op = operators.top(); operators.pop(); // Присваиваем и удаляем верхний оператор стека
				values.push(applyOperator(a, b, op)); // Результат операции обратно в стек
			}

			operators.push(c); // Помещаем текущий оператор в стек операторов
		}
	}

	values.push(stod(number)); // Помещаем последнее число в стек значений

	// Выполняем оставшиеся операции
	while (!operators.empty())
	{
		double b = values.top(); values.pop();
		double a = values.top(); values.pop();
		char op = operators.top(); operators.pop();
		values.push(applyOperator(a, b, op));
	}

	// Результат вычисления находится на вершине стека значений
	expression = formatResult(values.top());
	return true;
}

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
		cout << "failed:\n", WSAGetLastError();
		return false;
	}

	return true;
}

// Функция обработки клиентского потока
DWORD WINAPI ClientThread(LPVOID lpParam) 
{
	char buffer[1000];
	ClientData* pData = (ClientData*)lpParam;
	SOCKET clientSocket = pData->socket;
	int clientNumber = pData->client_number;
	
	bool flag = true;
	while (flag)
	{
		printCausedBy(recv(clientSocket, buffer, sizeof(buffer), NULL), "Recv");
		string expression(buffer);

		if (strcmp(buffer, "end")) // если не end
		{
			cout << "\nReceived from client #" << clientNumber << " expression: " << expression;
			if (!calculateExpression(expression))
				expression = "Incorrect expression!";
			cout << " Result: " << expression << endl;
			printCausedBy(send(clientSocket, expression.c_str(), expression.size() + 1, NULL), "Send");
		}
		else
		{
			cout << "\nClient " << clientNumber << " disconnected!\n\n";
			ReleaseSemaphore(hSemaphore, 1, NULL);
			flag = false;
		}
		
	}

	delete pData; // Освобождаем память
	return 0;
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
		return -1;
	}

	//Заполняем информацию об адресе сокета 
	SOCKADDR_IN addr;
	static int sizeOfAddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	//сокет для прослушвания порта 
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	
	//привязка адреса сокету  
	if (bind(sListen, (SOCKADDR*)&addr, sizeOfAddr) == SOCKET_ERROR)
	{
		printf("Error bind %d\n", WSAGetLastError());
		closesocket(sListen);
		WSACleanup();
		return -1;
	}
	
	//подкючение прослушивания с максимальной очередью 
	if (listen(sListen, SOMAXCONN) == SOCKET_ERROR) 
	{
		cerr << "Listen failed.\n";
		closesocket(sListen);
		WSACleanup();
		return -1;
	}

	// Создание семафора, контролирующего до 2 обращений одновременно 
	hSemaphore = CreateSemaphore(NULL, 2, 2, L"semaphore");
	if (hSemaphore == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		closesocket(sListen);
		WSACleanup();
		return -1;
	}

	cout << "Enter the number of clients from 1 to 10: ";
	int clients = read_int();
	
	//задаем информацию для окна открытия  
	STARTUPINFO si;//структура 
	PROCESS_INFORMATION pi;// структура с информацией о процессе 
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);// указываем размер 
	ZeroMemory(&pi, sizeof(pi));

	//создаём новые окна для клиентов 
	for (uint8_t i = 0; i < clients; i++)
	{
		if (!CreateProcess(
			L"C:\\Users\\Artem1\\Desktop\\4 семестр\\Операционные системы\\Лабы\\Лаба 5 ОС\\Lab_5_OS_Client\\x64\\Debug\\Lab_5_OS_Client.exe",   //  module name 
			NULL,        // Command line 
			NULL,           // Process handle not inheritable 
			NULL,           // Thread handle not inheritable 
			FALSE,          // Set handle inheritance to FALSE 
			CREATE_NEW_CONSOLE, //creation flags 
			NULL,           // Use parent's environment block 
			NULL,           // Use parent's starting directory  
			&si,            // Pointer to STARTUPINFO structure
			&pi)           // Pointer to PROCESS_INFORMATION structure
			)
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
			CloseHandle(hSemaphore);
			closesocket(sListen);
			WSACleanup();
			return -1;
		}
		Sleep(10);
	}

	//сокеты для соединения с клиентами
	vector <SOCKET> Sockets(clients); //вектор для сокетов 
	for (uint8_t i = 0; i < clients; i++) 
	{
		SOCKET clientSocket = accept(sListen, (SOCKADDR*)&addr, &sizeOfAddr);
		if (clientSocket == INVALID_SOCKET) // если ошибка
		{
			cerr << "Error accepting client connection: " << WSAGetLastError() << endl;
			CloseHandle(hSemaphore);
			closesocket(sListen);
			WSACleanup();
			return -1;		
		}
		Sockets[i] = clientSocket;
	}
	
	// Создаем клиентские потоки
	vector <HANDLE> clientThreads(clients);
	for (int i = 0; i < clients; ++i)
	{
		ClientData* pData = new ClientData;
		pData->socket = Sockets[i];
		pData->client_number = i + 1;
		clientThreads[i] = CreateThread(NULL, 0, ClientThread, pData, 0, NULL);

		// если ошибка при создании потока
		if (clientThreads[i] == NULL)
		{
			cerr << "\nError creating client thread " << i + 1 << ": " << GetLastError() << endl;
			delete pData;
			for (int j = 0; j < i; ++j)
			{
				TerminateThread(clientThreads[j], 0);
				CloseHandle(clientThreads[j]);
			}
			CloseHandle(hSemaphore);
			for (int i = 0; i < clients; ++i)
			{
				closesocket(Sockets[i]);
			}
			closesocket(sListen);
			WSACleanup();
			return -1;
		}
	}

	// Ждем когда все клиентские потоки завершатся
	WaitForMultipleObjects(clientThreads.size(), clientThreads.data(), TRUE, INFINITE);
	cout << "\nWork completed successfully!\n";

	CloseHandle(hSemaphore);
	for (int i = 0; i < clients; ++i) 
	{
		CloseHandle(clientThreads[i]);
		closesocket(Sockets[i]);
	}
	closesocket(sListen);
	WSACleanup();
	return 0;

}