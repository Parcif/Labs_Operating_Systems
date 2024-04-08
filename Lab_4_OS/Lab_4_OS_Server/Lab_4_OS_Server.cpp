#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <stack>
#define BUFSIZE 512
using namespace std;

typedef struct
{
	HANDLE hPipeInst;	// Дескриптор канала 
	string expression;	// Строка с выражением
} PIPEINST;

void print_time()
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	printf("%02d:%02d:%02d\t", lt.wHour, lt.wMinute, lt.wMilliseconds);
}

bool connectToClient(HANDLE hPipe)
{
	BOOL fConnected = ConnectNamedPipe(hPipe, NULL); // Пытаемся подключиться к каналу 

	if (fConnected)
	{
		print_time();
		cout << "[MESSAGE] Connected to the client!\n";
		return true;
	}
	else
	{
		print_time();
		cout << "[ERROR] ConnectNamedPipe failed with " << GetLastError() << "!\n";
		return false;
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

PIPEINST Pipe; // Одиночный экземпляр канала
/* PIPEINST содержит информацию о каждом канале, такую как дескриптор канала,
состояние канала, количество прочитанных байт и т.д.*/


int main()
{
	DWORD cbRead; // Количество прочитанных / записанных байт
	bool fSuccess; // Переменная для проверки корректности операций

	// Инициализируем канал
	Pipe.hPipeInst = CreateNamedPipe(
		L"\\\\.\\pipe\\mynamedpipe", // Имя канала
		PIPE_ACCESS_DUPLEX,	// Чтение и запись в канал
		PIPE_TYPE_MESSAGE |		// Канал передает сообщения, а не поток байт
		PIPE_READMODE_MESSAGE |	// Канал считывает сообщения
		PIPE_WAIT,		// Клиент будет ожидать, когда канал станет доступен
		1,	// Количество экземпляров канала
		BUFSIZE * 4,	// Выходной размер буфера
		BUFSIZE * 4,	// Входной размер буфера
		NMPWAIT_USE_DEFAULT_WAIT,	// Время ожидания клиента по умолчанию
		NULL);	// Атрибут защиты

	if (Pipe.hPipeInst == INVALID_HANDLE_VALUE) // Ошибка создания канала
	{
		print_time();
		cout << "[ERROR] CreateNamedPipe failed with " << GetLastError() << "!\n";
		return 1;
	}

	print_time();
	cout << "[MESSAGE] Pipe has been created!\n";

	// Подключение к клиенту
	if (!connectToClient(Pipe.hPipeInst)) // Если подключение не успешно
	{
		print_time();
		cout << "[ERROR] Connection error!\n";
		CloseHandle(Pipe.hPipeInst); // Закрытие дескриптора канала
		return 1;
	}
	
	// Обработка действий
	while (true)
	{
		bool clear_expression = false; // Флаг очистки выражения
		
		// ЧТЕНИЕ ДАННЫХ ИЗ КАНАЛА
		
		print_time();
		cout << "[MESSAGE] Waiting message from client!\n";

		char buffer[BUFSIZE];
		fSuccess = ReadFile(
			Pipe.hPipeInst,	// Канал
			buffer,			// Буфер для данных
			BUFSIZE,		// Размер буфера
			&cbRead,		// Количество считанных байт
			NULL);			// Не используем асинхронное чтение

		// При успешном завершении чтения 
		if (fSuccess && cbRead != 0)
		{
			print_time();
			cout << "[MESSAGE] Message from the client has been received!\n";

			if (!strcmp(buffer, "help"))
			{
				Pipe.expression = "Commands: clear, valid, calc, exit";
				clear_expression = true;
			}
			else if (!strcmp(buffer, "exit"))
			{
				print_time();
				cout << "[MESSAGE] Goodbye!\n";
				CloseHandle(Pipe.hPipeInst); // Закрытие дескриптора канала
				return 0;
			}
			else if (!strcmp(buffer, "clear")) // Очищаем строку с выражением
			{
				Pipe.expression.clear();
			}
			else if (!strcmp(buffer, "valid")) // Проверяем строку с выражением на корректность
			{
				if (!isValidExpression(Pipe.expression))
				{
					Pipe.expression = "Incorrect expression!";
					clear_expression = true;
				}
			
			}
			else if (!strcmp(buffer, "calc")) // Вычисляем выражение
			{
				if (!calculateExpression(Pipe.expression))
					Pipe.expression = "Incorrect expression!";
				clear_expression = true;
			}
			else
			{
				string message(buffer);
				Pipe.expression += message; // Добавляем к текущему выражению
			}

		}
		else // Иначе, если произошла ошибка, завершаем работу
		{
			print_time();
			cerr << "[ERROR] Reading error! " << GetLastError() << '\n';
			CloseHandle(Pipe.hPipeInst); // Закрытие дескриптора канала
			return 1;
		}

		// ЗАПИСЬ ДАННЫХ В КАНАЛ

		// Проверка размера выражения перед записью
		if (Pipe.expression.size() > BUFSIZE)
		{
			print_time();
			cout << "[ERROR] The numbers size exceeds the size of the buffer!\n";
			CloseHandle(Pipe.hPipeInst); // Закрытие дескриптора канала
			return 1;
		}

		// Записываем выражение в канал
		fSuccess = WriteFile(
			Pipe.hPipeInst,				// Канал
			Pipe.expression.c_str(),	// Передаваемая строка
			Pipe.expression.size() + 1,	// Размер передаваемых данных
			&cbRead,					// Количество записанных байт
			NULL);						// Не используем асинхронное чтение

		// Если запись не удалась, завершаем работу
		if (!fSuccess)
		{
			print_time();
			cerr << "[ERROR] Writing error! " << GetLastError() << '\n';
			CloseHandle(Pipe.hPipeInst); // Закрытие дескриптора канала
			return 1;
		}

		print_time();
		cout << "[MESSAGE] Result has been sent to the client!\n";

		// Если выражение ошибочно или после подсчета очищаем его
		if(clear_expression)
			Pipe.expression.clear();		

	}

	return 0;

}