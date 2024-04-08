#include <windows.h>  
#include <iostream>
#include <string>
#define BUFSIZE 512 
using namespace std;

void print_time() 
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	printf("%02d:%02d:%02d\t", lt.wHour, lt.wMinute, lt.wMilliseconds);
}


int main()
{
	HANDLE hPipe;		// Канал
	bool fSuccess = false;	// Переменная для проверки корректности операций 
	DWORD cbIO;		// Количество прочитанных / записанных байт

	// Подключаемся к каналу
	hPipe = CreateFile(
		L"\\\\.\\pipe\\mynamedpipe",  // Имя канала  
		GENERIC_READ |                // Чтение и запись из канала 
		GENERIC_WRITE,
		0,                            // Уровень общего доступа 
		NULL,                         // Атрибуты безопасности 
		OPEN_EXISTING,                // Открыть существующий 
		0,                            // Атрибуты файла 
		NULL);                        // Файл шаблона

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		print_time();
		cout << "[ERROR] Could not open pipe. GLE=" << GetLastError() << '\n';
		return 1;
	}

	string expression;

	while (expression != "exit")
	{
		print_time();
		cout << "[MESSAGE] Enter expression or command: ";
		getline(cin, expression);

		fSuccess = WriteFile(
			hPipe,	//Канал  
			expression.c_str(),	// Данные для передачи  
			expression.size() + 1,	// Количество переданных байт 
			&cbIO,	// Сколько байт передалось на самом деле  
			NULL);	// Не асинхронный вывод  

		if (!fSuccess)
		{
			print_time();
			cout << "[ERROR] WriteFile to pipe failed. GLE=" << GetLastError() << '\n';
			CloseHandle(hPipe);
			return 1;
		}

		if(expression != "exit")
		{
			// Читаем из канала все выражение
			char buffer[BUFSIZE];
			fSuccess = ReadFile(
				hPipe,	 // Канал
				buffer,	 // Куда записываем данные  
				BUFSIZE, // Какое кол-во байт записываем  
				&cbIO,	 // Сколько записали байт  
				NULL);	 // Не асинхронное чтение 

			if (!fSuccess)
			{
				print_time();
				cout << "[ERROR] ReadFile from pipe failed. GLE=" << GetLastError() << '\n';
				CloseHandle(hPipe);
				return 1;
			}

			print_time();
			cout << "[MESSAGE] Received from server: " << buffer << endl;

		}

	}

	print_time();
	cout << "[MESSAGE] Closing pipe and exit!\n";
	CloseHandle(hPipe);
	return 0;

}
