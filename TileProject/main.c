#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <wchar.h>
#include <locale.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
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
    wcscpy_s(fontInfo.FaceName, sizeof(fontInfo.FaceName) / sizeof(wchar_t), fontName);
    SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
}

void SetFontSize(int size) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_FONT_INFOEX fontInfo = { sizeof(CONSOLE_FONT_INFOEX) };
    GetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);

    fontInfo.dwFontSize.Y = size; // 폰트의 세로 크기를 변경합니다.

    SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
}

int WriteImageByWidth(const char* fileName, wchar_t** buffer, int width, int height) {

    IplImage* image = cvLoadImage(fileName, 3);

    if (!image) {
        printf("Could not open Image file\n");
        printf("%s", fileName);
    }

    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
    cvResize(image, resizedImage, CV_INTER_LINEAR);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            CvScalar value = cvGet2D(resizedImage, y, x);
            unsigned char R = value.val[2];
            unsigned char G = value.val[1];
            unsigned char B = value.val[0];
                    
            wchar_t pixel[26];
            swprintf_s(pixel, sizeof(pixel) / sizeof(wchar_t), L"\x1b[38;2;%03hhu;%03hhu;%03hhum█ \x1b[0m", R, G, B);
            wcscat_s(buffer[y], width * 25 + 1, pixel);
        }
    }
}

void Init() {
    setlocale(LC_ALL, ""); // 언어 설정을 현재 컴퓨터의 언어설정으로 변경
    wprintf(L"언어 설정 변경 됨\n");
    EnableVitrualTerminal(true); // Virtual Terminal 활성화, ANSI escape code 사용 가능하게 만듦
    wprintf(L"Virtual Terminal 활성화\n");
    _setmode(_fileno(stdout), _O_U16TEXT); // 기본 변환 모드를 유니코드로 설정
    wprintf(L"출력모드 유니코드로 설정\n");
    SetConsoleFont(L"NSimSun"); // 폰트를 NsimSun으로 변경
    wprintf(L"현재 폰트 NSimSun으로 변경\n");
    SetFontSize(1);
}

int	main() {
    Init();

    int width = 300;
    int height = 200;

    wchar_t** buffer = (wchar_t**)malloc(sizeof(wchar_t*) * height);

    for (int i = 0; i < height; i++)
    {
        buffer[i] = (wchar_t*)malloc(sizeof(wchar_t) * (width * 25 + 1));
    }

    COORD textPos = { 0, 0 };

    for (int i = 0; i < height; i++)
    {
        buffer[i][0] = '\0';
    }

    WriteImageByWidth("TestImage.png", buffer, width, height);

    for (int i = 0; i < height; i++)
    {
        wprintf(L"%ls\n", buffer[i]);
    }
}