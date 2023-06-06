#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <wchar.h>
#include <locale.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <io.h>
#include <fcntl.h>

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 220

enum ConsoleColor {
    BLACK,
    DARK_BLUE,
    DARK_GREEN,
    DARK_CYAN,
    DARK_RED,
    DARK_MAGENTA,
    DARK_YELLOW,
    GRAY,
    DARK_GRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    WHITE,
    UNDEFINED
};

typedef struct {
    enum ConsoleColor colorTag;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} DefineColor;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

DefineColor ConsoleColors[] = {
    {BLACK, 0, 0, 0},
    {DARK_BLUE, 0, 0, 128},
    {DARK_GREEN, 0, 128, 0},
    {DARK_CYAN, 0, 128, 128},
    {DARK_RED, 128, 0, 0},
    {DARK_MAGENTA, 128, 0, 128},
    {DARK_YELLOW, 128, 128, 0},
    {GRAY, 192, 192, 192},
    {DARK_GRAY, 128, 128, 128},
    {BLUE, 0, 0, 255},
    {GREEN, 0, 255, 0},
    {CYAN, 0, 255, 255},
    {RED, 255, 0, 0},
    {MAGENTA, 255, 0, 255},
    {YELLOW, 255, 255, 0},
    {WHITE, 255, 255, 255}
};

void SetCursorPosition(int x, int y) {
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void SetCursorVisible(bool isVisible) {
	CONSOLE_CURSOR_INFO cursorInfo = { 0, };
	cursorInfo.dwSize = 1;
	cursorInfo.bVisible = isVisible;
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

    return 0;
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
    SetConsoleFont(L"NSimSun"); // 폰트를 NsimSun으로 변경
}

double colorDistance(Color color1, DefineColor color2) {
    int dr = color1.r - color2.r;
    int dg = color1.g - color2.g;
    int db = color1.b - color2.b;

    return sqrt(dr * dr + dg * dg + db * db);
}

DefineColor FindClosestConsoleColor(Color inputColor) {
    DefineColor closestColor = ConsoleColors[0];
    double minDistance = colorDistance(inputColor, closestColor);
    for (int i = 1; i < sizeof(ConsoleColors) / sizeof(ConsoleColors[0]); i++)
    {
        double distance = colorDistance(inputColor, ConsoleColors[i]);
        if (distance < minDistance) {
            minDistance = distance;
            closestColor = ConsoleColors[i];
        }
    }
    closestColor.a = inputColor.a;

    return closestColor;
}

void DrawPixel(CHAR_INFO* buffer, Color color, int x, int y) {
    if (color.a < 255)
        return;

    DefineColor closestColor = FindClosestConsoleColor(color);

    if(x >= 0 && y >= 0 && x < SCREEN_WIDTH && y < SCREEN_HEIGHT){
        x *= 2;
        buffer[y * (SCREEN_WIDTH * 2) + x].Char.UnicodeChar = L'█';
        buffer[y * (SCREEN_WIDTH * 2) + (x + 1)].Char.UnicodeChar = L' ';
        buffer[y * (SCREEN_WIDTH * 2) + x].Attributes = closestColor.colorTag;
        buffer[y * (SCREEN_WIDTH * 2) + (x + 1)].Attributes = closestColor.colorTag;
    }
}

void DrawPixelConsoleColor(CHAR_INFO* buffer, enum ConsoleColor color, int x, int y) {
    if (x >= 0 && y >= 0 && x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        x *= 2;
        buffer[y * (SCREEN_WIDTH * 2) + x].Char.UnicodeChar = L'█';
        buffer[y * (SCREEN_WIDTH * 2) + (x + 1)].Char.UnicodeChar = L' ';
        buffer[y * (SCREEN_WIDTH * 2) + x].Attributes = color;
        buffer[y * (SCREEN_WIDTH * 2) + (x + 1)].Attributes = color;
    }
}

void DrawSprite(const char* fileName, CHAR_INFO* buffer, int width, int height, int _x, int _y) {
    IplImage* image = cvLoadImage(fileName, CV_LOAD_IMAGE_UNCHANGED);

    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);

    if (!image) {
        cvSet(resizedImage, CV_RGB(255, 0, 255), NULL);
    }
    else {
        cvResize(image, resizedImage, CV_INTER_LINEAR);
    }

    for (int i = 0; i < width * height; i++)
    {
        int x = i % width;
        int y = i / width;

        CvScalar value = cvGet2D(resizedImage, y, x);
        unsigned char R = value.val[2];
        unsigned char G = value.val[1];
        unsigned char B = value.val[0];
        unsigned char A = value.val[3];
        Color color = {R, G, B, A};

        DrawPixel(buffer, color, x + _x, y + _y);
    }
    cvReleaseImage(&image);
    cvReleaseImage(&resizedImage);
}

void ClearBuffer(CHAR_INFO* buffer) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        int x = i % SCREEN_WIDTH;
        int y = i / SCREEN_WIDTH;

        DrawPixelConsoleColor(buffer, WHITE, x, y);
    }
}

void FillBuffer(CHAR_INFO* buffer, enum ConsoleColor color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        int x = i % SCREEN_WIDTH;
        int y = i / SCREEN_WIDTH;

        DrawPixelConsoleColor(buffer, color, x, y);
    }
}

void PlayVideo(const char* videoName, CHAR_INFO* buffer, int width, int height, int _x, int _y) {

    CvCapture* capture = cvCreateFileCapture(videoName);

    if (!capture)
    {
        return -1;
    }

    IplImage* frame = 0;
    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

    float frameRate = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
    int frameCount = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정

    int currentFrame = 0;

    DWORD lastTick = 0;
    DWORD lastInputTick = 0;
    DWORD lastPauseTick = 0;
    while (currentFrame < frameCount)
    {
        DWORD currentTick = GetTickCount64();

        if (currentTick - lastTick < 24)
            continue;
        lastTick = currentTick;

        frame = cvQueryFrame(capture); // 프레임을 가져옴

        if (!frame)
            break;

        cvResize(frame, resizedImage, CV_INTER_LINEAR);

        for (int i = 0; i < width * height; i++)
        {
            int x = i % width;
            int y = i / width;

            CvScalar value = cvGet2D(resizedImage, y, x);
            unsigned char R = value.val[2];
            unsigned char G = value.val[1];
            unsigned char B = value.val[0];
            Color color = {R, G, B, 255};

            DrawPixel(buffer, color, x + _x, y + _y);
        }

        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
        currentFrame++;
    }

    // 메모리 할당 해제
    cvReleaseCapture(&capture);
    cvReleaseImage(&resizedImage);
}

void Init() {
    char commandMessage[150] = "";
    SetFontSize(4);
    sprintf_s(commandMessage, sizeof(commandMessage), "mode con cols=%d lines=%d", SCREEN_WIDTH * 2, SCREEN_HEIGHT);
    system(commandMessage);
    SetCursorVisible(false);
    setlocale(LC_ALL, ""); // 언어 설정을 현재 컴퓨터의 언어설정으로 변경
    EnableVitrualTerminal(true); // Virtual Terminal 활성화, ANSI escape code 사용 가능하게 만듦
    _setmode(_fileno(stdout), _O_U16TEXT); // 기본 변환 모드를 유니코드로 설정
}

int	main() {
    Init();

    // 동적으로 CHAR_INFO 배열 생성
    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정


    PlayVideo("Assets\\Videos\\Title.mp4", buffer, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
    while (true)
    {
        FillBuffer(buffer, GREEN);
        DrawSprite("Assets\\Sprites\\Glass.png", buffer, 16, 16, SCREEN_WIDTH / 2 - 8 - 16, SCREEN_HEIGHT / 2 - 8);
        DrawSprite("Assets\\Sprites\\Player.png", buffer, 16, 16, SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 8);
        DrawSprite("Assets\\Sprites\\MissingTex.png", buffer, 16, 16, SCREEN_WIDTH / 2 - 8 + 16, SCREEN_HEIGHT / 2 - 8);
        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }

    // 동적으로 할당된 배열 해제
    free(buffer);
    SetCursorPosition(0, 0);
    return 0;
}