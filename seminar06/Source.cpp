#include <Windows.h>
#include <vector>
#include <fstream>
#include <sstream>
#include "json.hpp"
using json = nlohmann::json;

std::string configFile = "settings.json"; //Конфигурационный файл

int baseWindowWidth = 320, baseWindowHeight = 240; //Размер окна по умолчанию
int minWindowWidth = 200, minWindowHeight = 200; //Минимальный размер окна

COLORREF backColor = RGB(45, 73, 255); //Синий цвет окна по умолчанию
HBRUSH hBrushBackground = CreateSolidBrush(backColor); //Кисть для покраски окна
COLORREF lineColor = RGB(255, 48, 55); //Красный цвет линий по умолчанию
COLORREF xColor = RGB(255, 255, 255); //Цвет крестика по умолчанию
COLORREF oColor = RGB(0, 0 ,0); //Цвет нолика по умолчанию

int gridSize = 3; //Размер сетки по умолчанию
const int MAX_GRID_SIZE = 10; //Максимальный размер сетки
char board[MAX_GRID_SIZE][MAX_GRID_SIZE]; //Массив для сохранения X и O

const wchar_t сlassName[] = L"TicTacToeWindowClass";
HANDLE hMapping = NULL;
char* sharedMemory = NULL;
int sharedMemorySize;
UINT WM_UPDATE_BOARD = RegisterWindowMessage(L"TicTacToe_UpdateBoard");

//Инициализация общей памяти
void InitSharedMemory() {
	sharedMemorySize = MAX_GRID_SIZE * MAX_GRID_SIZE;
	hMapping = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sharedMemorySize,
		L"Local\\TicTacToeSharedMemory");

	if (hMapping == NULL) {
		MessageBox(NULL, L"Не удалось создать разделяемую память", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	bool isFirstInstance = (GetLastError() != ERROR_ALREADY_EXISTS);
	sharedMemory = (char*)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, sharedMemorySize);

	if (sharedMemory == NULL) {
		MessageBox(NULL, L"Не удалось отобразить разделяемую память", L"Ошибка", MB_OK | MB_ICONERROR);
		CloseHandle(hMapping);
		hMapping = NULL;
		return;
	}

	if (isFirstInstance) {
		memset(sharedMemory, '.', sharedMemorySize);
	}

	// Копируем данные из общей памяти
	for (int y = 0; y < gridSize; ++y) {
		for (int x = 0; x < gridSize; ++x) {
			board[y][x] = sharedMemory[y * MAX_GRID_SIZE + x];
		}
	}
}

//обновление доски
void UpdateBoard() {
	if (!sharedMemory) 
		return;

	for (int y = 0; y < gridSize; ++y) {
		for (int x = 0; x < gridSize; ++x) {
			board[y][x] = sharedMemory[y * MAX_GRID_SIZE + x];
		}
	}
}

// Очистка ресурсов
void CleanupSharedMemory() {
	if (sharedMemory) {
		UnmapViewOfFile(sharedMemory);
		sharedMemory = NULL;
	}
	if (hMapping) {
		CloseHandle(hMapping);
		hMapping = NULL;
	}
}

// Отправляем сообщение всем окнам нашего класса
void NotifyAllWindows(HWND hwnd) {
	
	EnumWindows([](HWND hwndTarget, LPARAM lParam) -> BOOL {
		wchar_t className[256];
		GetClassName(hwndTarget, className, 256);

		if (wcscmp(className, сlassName) == 0 && hwndTarget != (HWND)lParam) {
			PostMessage(hwndTarget, WM_UPDATE_BOARD, 0, 0);
		}
		return TRUE;
		}, 
	(LPARAM)hwnd);
}

//проверяем, что строка состоит только из цифр
bool IsPositiveInteger(LPCWSTR str) {

	for (int i = 0; str[i] != L'\0'; i++) {
		if (!iswdigit(str[i])) 
			return false;
	}

	return true;
}

//загружаем настройки
void LoadConfig() {
	std::ifstream file(configFile);
	if (!file.is_open()) return;

	try
	{
		json config;
		file >> config;

		// Проверка gridSize
		if (config.contains("gridSize") && config["gridSize"].is_number_integer() && config["gridSize"] > 0 && config["gridSize"] <= 10) {
			gridSize = config["gridSize"]; 
		}
		
		// Проверка winSize 
		if (config.contains("winSize") && config["winSize"].is_array() && config["winSize"].size() == 2) { 
			if (config["winSize"][0].is_number_integer() &&
				config["winSize"][1].is_number_integer() && 
				config["winSize"][0] > minWindowWidth &&
				config["winSize"][1] > minWindowHeight) {
				baseWindowWidth = config["winSize"][0]; 
				baseWindowHeight = config["winSize"][1]; 
			}
		}
		
		// Проверка backColor
		if (config.contains("backColor") && config["backColor"].is_array() && config["backColor"].size() == 3) { 
			if (config["backColor"][0].is_number_integer() &&
				config["backColor"][1].is_number_integer() && 
				config["backColor"][2].is_number_integer()) { 
				int r = config["backColor"][0], 
					g = config["backColor"][1], 
					b = config["backColor"][2]; 
				if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) { 
					backColor = RGB(r, g, b); 
				}
			}
		}
		hBrushBackground = CreateSolidBrush(backColor);
		
		// Проверка lineColor
		if (config.contains("lineColor") && config["lineColor"].is_array() && config["lineColor"].size() == 3) {
			if (config["lineColor"][0].is_number_integer() &&
				config["lineColor"][1].is_number_integer() &&
				config["lineColor"][2].is_number_integer()) { 
				int r = config["lineColor"][0], 
					g = config["lineColor"][1], 
					b = config["lineColor"][2];
				if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
					lineColor = RGB(r, g, b);
				}
			}
		}
	}
	catch (...) { 
		MessageBox(NULL, L"Не получилось прочитать существующий файл с настройками, будут используются настройки по умолчанию. \n\nПосле закрытия приложения установленные настройки сохранятся и это ошбика пропадёт.", L"Ошибка чтения", MB_OK | MB_ICONWARNING);
	}
}

//сохраняем настройки
void SaveConfig(HWND hwnd) {
	RECT winrect;
	GetWindowRect(hwnd, &winrect); 
	int winWidth = winrect.right - winrect.left;
	int winHeight = winrect.bottom - winrect.top;

	json config =  
	{
		{"gridSize", gridSize},
		{"winSize", { winWidth, winHeight }},
		{"backColor", { GetRValue(backColor), GetGValue(backColor), GetBValue(backColor) }},
		{"lineColor", { GetRValue(lineColor), GetGValue(lineColor), GetBValue(lineColor) }}
	};

	std::ofstream file(configFile);
	file << config.dump(4);
}

// Закрытие приложения
void CloseApp(HWND hwnd) {
	RECT winrect;
	GetWindowRect(hwnd, &winrect);

	SaveConfig(hwnd); //Сохранение конфига
	CleanupSharedMemory(); //Очистка общей памяти
	PostQuitMessage(0); //Выход
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInt, HINSTANCE hPrev, PWSTR pCmdLine, int nShow) {

	LoadConfig(); //загружаем настройки перед созданием окна

	WNDCLASS SoftwareWindClass = { 0 };
	SoftwareWindClass.hIcon = LoadIcon(NULL, IDI_QUESTION);
	SoftwareWindClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	SoftwareWindClass.hInstance = hInt;
	SoftwareWindClass.lpszClassName = сlassName;
	SoftwareWindClass.hbrBackground = hBrushBackground;
	SoftwareWindClass.lpfnWndProc = WindowProc;

	//считываем аргументы командной строки
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc > 2) {
		MessageBoxW(NULL, L"Введено больше 1-го аргумента! \nВ качестве размера поля будет использовано значение из конфигурационного файла либо число 3 по умолчанию.", L"Неверный ввод", MB_OK | MB_ICONWARNING);
	}
	else if(argc == 2 && !IsPositiveInteger(argv[1])) {
		MessageBoxW(NULL, L"Введено неположительное целое число в качестве аргумента!\nВ качестве размера поля будет использовано значение из конфигурационного файла либо число 3 по умолчанию.", L"Неверный ввод", MB_OK | MB_ICONWARNING);
	}
	else if(argc == 2) {
		if (argv[1] == 0)
			MessageBoxW(NULL, L"В качестве размера поля не может быть использовано число 0.\n Будет использовано значение из конфигурационного файла либо число 3 по умолчанию.", L"Неверный ввод", MB_OK | MB_ICONWARNING);
		else if (_wtoi(argv[1]) > MAX_GRID_SIZE)
			MessageBoxW(NULL, L"Максимальный размер поля: 10.\n Будет использовано значение из конфигурационного файла либо число 3 по умолчанию.", L"Неверный ввод", MB_OK | MB_ICONWARNING);
		else
			gridSize = _wtoi(argv[1]);
	}
	
	if (!RegisterClassW(&SoftwareWindClass)) {
		return -1;
	}

	HWND hwnd = CreateWindowW(
		сlassName,
		L"Игра крестики нолики",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		200, 200, //координаты появления
		baseWindowWidth, baseWindowHeight, //размер окна
		NULL,
		NULL,
		hInt,
		NULL
	);

	if (!hwnd) {
		MessageBox(NULL, L"Не удалось создать окно", L"Ошибка", MB_OK | MB_ICONERROR);
		return -1;
	}

	MSG SoftwareMsg = { 0 };
	while (GetMessage(&SoftwareMsg, NULL, NULL, NULL)) {
		TranslateMessage(&SoftwareMsg);
		DispatchMessage(&SoftwareMsg);
	}

	return 0;
}

//Рисование сетки
void DrawLines(HWND hwnd, HDC hdc, int windowWidth, int windowHeight) {

	if (gridSize == 0) 
		return;

	HPEN hPen = CreatePen(PS_DOT, 4, lineColor);
	SelectObject(hdc, hPen);

	for (size_t i = 1; i < gridSize; i++)
	{
		MoveToEx(hdc, windowWidth / gridSize * i, 0, NULL);
		LineTo(hdc, windowWidth / gridSize * i, windowHeight);

		MoveToEx(hdc, 0, windowHeight / gridSize * i, NULL);
		LineTo(hdc, windowWidth, windowHeight / gridSize * i);
	}

	//удаляем кисть
	DeleteObject(hPen); 
}

//Рисование крестика
void DrawX(HWND hwnd, HDC hdc, int x, int y, int cellWidth, int cellHeight) {

	//координаты ячейки
	int cellX = (x / cellWidth) * cellWidth;
	int cellY = (y / cellHeight) * cellHeight;

	HPEN hPen = CreatePen(PS_SOLID, 8, xColor);
	SelectObject(hdc, hPen);

	MoveToEx(hdc, cellX + 10, cellY + 10, NULL);
	LineTo(hdc, cellX + cellWidth - 10, cellY + cellHeight - 10);

	MoveToEx(hdc, cellX + 10, cellY + cellHeight - 10, NULL);
	LineTo(hdc, cellX + cellWidth - 10, cellY + 10);

	//удаляем кисть
	DeleteObject(hPen);
}

//Рисование нолика
void DrawO(HWND hwnd, HDC hdc, int x, int y, int cellWidth, int cellHeight) {

	//координаты ячейки
	int cellX = (x / cellWidth) * cellWidth;
	int cellY = (y / cellHeight) * cellHeight;

	HPEN hPen = CreatePen(PS_SOLID, 8, oColor);
	HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH); // Отключаем заливку

	SelectObject(hdc, hPen);
	SelectObject(hdc, hBrush);

	Ellipse(hdc, cellX + 10, cellY + 10, cellX + cellWidth - 10, cellY + cellHeight - 10);

	//удаляем кисть
	DeleteObject(hPen);
	DeleteObject(hBrush);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	//получаем размеры клиентской области окна
	RECT rect; 
	GetClientRect(hwnd, &rect); 
	int winWidth = rect.right; 
	int winHeight = rect.bottom; 

	int cellWidth = winWidth / gridSize; //ширина клетки
	int cellHeight = winHeight / gridSize; //высота клетки

	switch (uMsg) {
	case WM_CREATE: {
		InitSharedMemory(); 
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}
	case WM_USER + 1: {
		for (int y = 0; y < gridSize; ++y)
			for (int x = 0; x < gridSize; ++x)
				board[y][x] = sharedMemory[y * gridSize + x];
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		//получаем координаты клика
		POINT point = { 0 };
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);

		//получаем координаты клетки, куда кликнули
		int boardX = point.x / cellWidth;
		int boardY = point.y / cellHeight;
		if (boardX >= gridSize - 1)
			boardX = gridSize - 1;
		if (boardY >= gridSize - 1)
			boardY = gridSize - 1;

		// Обновляем общую память
		if (sharedMemory) {
			if (uMsg == WM_LBUTTONDOWN) {
				sharedMemory[boardY * MAX_GRID_SIZE + boardX] = 'O';
			}
			else {
				sharedMemory[boardY * MAX_GRID_SIZE + boardX] = 'X';
			}
		}

		// Обновляем текущую доску
		UpdateBoard();

		// Оповещаем все окна об обновлении
		NotifyAllWindows(hwnd);

		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		DrawLines(hwnd, hdc, winWidth, winHeight);

		for (int y = 0; y < gridSize; y++) {
			for (int x = 0; x < gridSize; x++) {

				int left = x * cellWidth; 
				int top = y * cellHeight; 
				int right = left + cellWidth; 
				int bottom = top + cellHeight; 

				if (board[y][x] == 'O')
					DrawO(hwnd, hdc, left, top, cellWidth, cellHeight);

				else if (board[y][x] == 'X') {
					DrawX(hwnd, hdc, left, top, cellWidth, cellHeight);
				}
			}
		}

		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_KEYDOWN: 
	{
		switch (wParam) {

		case VK_ESCAPE:
		{
			CloseApp(hwnd); 
			break;
		}
		case 'Q':
		{
			if ((GetKeyState(VK_CONTROL) & 0x8000)) {
				CloseApp(hwnd); 
			}
			break;
		}
		case 'C':
			if ((GetKeyState(VK_SHIFT) & 0x8000)) {
				STARTUPINFOW si = { 0 }; 
				PROCESS_INFORMATION pi = { 0 };
				CreateProcessW(L"c:\\windows\\system32\\notepad.exe", 
					NULL, 0, 0, 0, 0, 0, 0, &si, &pi);
			}
			break;
		case VK_RETURN:
			backColor = RGB(rand() % 256, rand() % 256, rand() % 256); // Случайный цвет

			DeleteObject(hBrushBackground);
			hBrushBackground = CreateSolidBrush(backColor);

			SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushBackground); 
			InvalidateRect(hwnd, NULL, TRUE); 
			break;
		}
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);

		int r = GetRValue(lineColor); 
		int g = GetGValue(lineColor); 
		int b = GetBValue(lineColor);  

		int step = 12;
		// Изменяем цвет в зависимости от направления прокрутки
		if (delta > 0) {
			r = (r + step) % 256;
			g = (g + step / 2) % 256;
			b = (b + step / 3) % 256;
		}
		else {
			r = (r - step + 256) % 256;
			g = (g - step / 2 + 256) % 256;
			b = (b - step / 3 + 256) % 256;
		}

		lineColor = RGB(r, g, b); 
		InvalidateRect(hwnd, NULL, TRUE); 

		return 0;
	}
	case WM_GETMINMAXINFO: { 
		MINMAXINFO* pMinMax = (MINMAXINFO*)lParam; 
		pMinMax->ptMinTrackSize.x = minWindowWidth; // Минимальная ширина 
		pMinMax->ptMinTrackSize.y = minWindowHeight; // Минимальная высота 
		return 0;
	}
	case WM_SIZE: {
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	case WM_DESTROY: {
		CloseApp(hwnd);
		return 0;
	}
	default: {
		if (uMsg == WM_UPDATE_BOARD) {
			UpdateBoard();
			InvalidateRect(hwnd, NULL, TRUE);
			return 0;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
}