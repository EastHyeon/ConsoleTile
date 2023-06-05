#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <wchar.h>
#include <locale.h>
#include <opencv/cv.h>
#include <io.h>
#include <fcntl.h>

void SetCursorPosition(int x, int y) {
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void SetCursorHide() {
	CONSOLE_CURSOR_INFO cursorInfo = { 0, };
	cursorInfo.dwSize = 1;
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void EnableVitrualTerminal(bool flag) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;

    GetConsoleMode(hOut, &dwMode);
    if(flag)
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    else
        dwMode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    return 0;\
}

void SetConsoleFont(const wchar_t* fontName) {
    // 현재 콘솔창의 핸들을 가져옵니다.
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // 현재 콘솔창의 정보를 가져옵니다.
    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    GetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
    wcscpy_s(fontInfo.FaceName, sizeof(fontInfo.FaceName) / sizeof(wchar_t), L"Consolas");
    SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
}

void Init() {
    setlocale(LC_ALL, ""); // 언어 설정을 현재 컴퓨터의 언어설정으로 변경
    Sleep(800);
    wprintf(L"언어 설정 변경 됨\n");
    EnableVitrualTerminal(true); // Virtual Terminal 활성화, ANSI escape code 사용 가능하게 만듦
    Sleep(800);
    wprintf(L"Virtual Terminal 활성화\n");
    _setmode(_fileno(stdout), _O_U16TEXT); // 기본 변환 모드를 유니코드로 설정
    Sleep(800);
    wprintf(L"출력모드 유니코드로 설정\n");
    SetConsoleFont(L"NSimSun"); // 폰트를 NsimSun으로 변경
    Sleep(800);
    wprintf(L"현재 폰트 NSimSun으로 변경\n");
}

int	main() {
    Init();
    wprintf(L"\x1b[38;2;255;0;0m Red █ \x1b[0m");
}