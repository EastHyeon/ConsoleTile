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

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

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

enum ObjectID {
    AIR,
    SOLID,
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

typedef struct {
    float x;
    float y;
    float z;
} Vector3;

typedef struct {
    float x;
    float y;
    void (*normalize)(struct Vector2);
} Vector2;

void normalize(Vector2 vector2) {
}

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

void EnableVitrualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode;

    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    return 0;
}

void DisableQuickEditMode() {
    HANDLE hOut = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleModePrev;

    GetConsoleMode(hOut, &consoleModePrev);
    SetConsoleMode(hOut, consoleModePrev & ~ENABLE_QUICK_EDIT_MODE);
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

void DrawTextSprite(const char* textSpriteAdress, CHAR_INFO* buffer, enum ConsoleColor color, int index, int _x, int _y) {
    IplImage* image = cvLoadImage(textSpriteAdress, CV_LOAD_IMAGE_UNCHANGED);

    int width = 5;
    int height = 7;

    if (!image)
        return;    

    int spriteWidth = image->width;

    index *= 5;

    if (index > spriteWidth - 5)
        index = 0;

    for (int i = 0; i < width * height; i++)
    {
        int indexX = (i % width) + index;
        int x = (i % width);
        int y = (i / width);

        CvScalar value = cvGet2D(image, y, indexX);
        unsigned char A = value.val[3];

        if (A > 254)
            DrawPixelConsoleColor(buffer, color, x + _x, y + _y);
    }
    cvReleaseImage(&image);
}

void ClearBuffer(CHAR_INFO* buffer) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        int x = i % SCREEN_WIDTH;
        int y = i / SCREEN_WIDTH;

        DrawPixelConsoleColor(buffer, WHITE, x, y);
    }
}

void InitColiderBuffer(enum ObjectID* buffer) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    {
        int x = i % SCREEN_WIDTH;
        int y = i / SCREEN_WIDTH;

        buffer[y * (SCREEN_WIDTH * 2) + x] = AIR;
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

void DrawChar(char character, CHAR_INFO* buffer, enum ConsoleColor color, int _x, int _y) {
    switch (character)
    {
    case 'A':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 1, _x, _y);
        break;
    case 'B':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 2, _x, _y);
        break;
    case 'C':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 3, _x, _y);
        break;
    case 'D':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 4, _x, _y);
        break;
    case 'E':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 5, _x, _y);
        break;
    case 'F':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 6, _x, _y);
        break;
    case 'G':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 7, _x, _y);
        break;
    case 'H':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 8, _x, _y);
        break;
    case 'I':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 9, _x, _y);
        break;
    case 'J':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 10, _x, _y);
        break;
    case 'K':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 11, _x, _y);
        break;
    case 'L':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 12, _x, _y);
        break;
    case 'M':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 13, _x, _y);
        break;
    case 'N':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 14, _x, _y);
        break;
    case 'O':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 15, _x, _y);
        break;
    case 'P':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 16, _x, _y);
        break;
    case 'Q':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 17, _x, _y);
        break;
    case 'R':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 18, _x, _y);
        break;
    case 'S':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 19, _x, _y);
        break;
    case 'T':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 20, _x, _y);
        break;
    case 'U':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 21, _x, _y);
        break;
    case 'V':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 22, _x, _y);
        break;
    case 'W':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 23, _x, _y);
        break;
    case 'X':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 24, _x, _y);
        break;
    case 'Y':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 25, _x, _y);
        break;
    case 'Z':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 26, _x, _y);
        break;
    case 'a':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 27, _x, _y);
        break;
    case 'b':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 28, _x, _y);
        break;
    case 'c':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 29, _x, _y);
        break;
    case 'd':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 30, _x, _y);
        break;
    case 'e':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 31, _x, _y);
        break;
    case 'f':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 32, _x, _y);
        break;
    case 'g':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 33, _x, _y);
        break;
    case 'h':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 34, _x, _y);
        break;
    case 'i':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 35, _x, _y);
        break;
    case 'j':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 36, _x, _y);
        break;
    case 'k':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 37, _x, _y);
        break;
    case 'l':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 38, _x, _y);
        break;
    case 'm':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 39, _x, _y);
        break;
    case 'n':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 40, _x, _y);
        break;
    case 'o':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 41, _x, _y);
        break;
    case 'p':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 42, _x, _y);
        break;
    case 'q':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 43, _x, _y);
        break;
    case 'r':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 44, _x, _y);
        break;
    case 's':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 45, _x, _y);
        break;  
    case 't':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 46, _x, _y);
        break;  
    case 'u':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 47, _x, _y);
        break;  
    case 'v':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 48, _x, _y);
        break;  
    case 'w':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 49, _x, _y);
        break;  
    case 'x':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 50, _x, _y);
        break;  
    case 'y':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 51, _x, _y);
        break;  
    case 'z':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 52, _x, _y);
        break;
    case '0':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 53, _x, _y);
        break;
    case '1':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 54, _x, _y);
        break;
    case '2':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 55, _x, _y);
        break;
    case '3':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 56, _x, _y);
        break;
    case '4':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 57, _x, _y);
        break;
    case '5':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 58, _x, _y);
        break;
    case '6':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 59, _x, _y);
        break;
    case '7':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 60, _x, _y);
        break;
    case '8':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 61, _x, _y);
        break;
    case '9':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 62, _x, _y);
        break;
    case '!':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 63, _x, _y);
        break;
    case '?':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 64, _x, _y);
        break;
    case ',':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 65, _x, _y);
        break;
    case '.':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 66, _x, _y);
        break;
    case ':':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 67, _x, _y);
        break;
    case ';':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 68, _x, _y);
        break;
    case '#':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 69, _x, _y);
        break;
    case '(':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 70, _x, _y);
        break;
    case ')':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 71, _x, _y);
        break;
    case '[':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 72, _x, _y);
        break;
    case ']':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 73, _x, _y);
        break;
    case '<':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 74, _x, _y);
        break;
    case '>':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 75, _x, _y);
        break;
    case '-':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 76, _x, _y);
        break;
    case '+':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 77, _x, _y);
        break;
    case '=':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 78, _x, _y);
        break;
    case '|':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 79, _x, _y);
        break;
    case '_':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 80, _x, _y);
        break;
    case '%':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 81, _x, _y);
        break;
    case '/':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 82, _x, _y);
        break;
    case '\'':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 83, _x, _y);
        break;
    case '\"':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 84, _x, _y);
        break;
    case '@':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 85, _x, _y);
        break;
    case '~':
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 86, _x, _y);
        break;
    case ' ':
        break;
    default:
        DrawTextSprite("Assets\\Sprites\\Text.png", buffer, color, 0, _x, _y);
        break;
    }
}

void DrawString(const char* string, CHAR_INFO* buffer, enum ConsoleColor color, int _x, int _y) {
    for (int i = 0; i < strlen(string); i++)
    {
        DrawChar(string[i], buffer, color, _x + (i * 6), _y);
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
    EnableVitrualTerminal(); // Virtual Terminal 활성화, ANSI escape code 사용 가능하게 만듦
    DisableQuickEditMode(); // Quick Edit Mode 비활성화
    _setmode(_fileno(stdout), _O_U16TEXT); // 기본 변환 모드를 유니코드로 설정
    wchar_t Title[50] = L"BitEngine";
    SetConsoleTitleW(Title);
}

int	main() {
    Init();

    POINT point;
    HWND hWnd;
    int width = 0;
    RECT window_size;

    point.x = 0;
    point.y = 0;

    // 동적으로 CHAR_INFO 배열 생성
    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);

    enum ObjectID* coliderBuffer = (enum ObjectID*)malloc(sizeof(enum ObjectID) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정
    
    int playerPosX = 0;
    int playerPosY = 0;

    Vector2 vector2 = { 0, 0 };

    DWORD currentTick = 0;
    DWORD lastInputTick = 0;

    PlayVideo("Assets\\Videos\\Title.mp4", buffer, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
    while (true)
    {
        GetCursorPos(&point);
        hWnd = WindowFromPoint(point);
        hWnd = WindowFromPoint(point);
        ScreenToClient(hWnd, &point);
        GetWindowRect(hWnd, &window_size);
        width = (window_size.right - window_size.left);

        currentTick = GetTickCount64();
        if (currentTick - lastInputTick > 10) {
            if(GetAsyncKeyState(VK_UP) & 0x8001 || GetAsyncKeyState(VK_W) & 0x8001) {
                if(playerPosY  > 0)
                    playerPosY--;
            }
            if(GetAsyncKeyState(VK_DOWN ) & 0x8001 || GetAsyncKeyState(VK_S) & 0x8001) {
                if(playerPosY + 1 < SCREEN_HEIGHT - 16)
                    playerPosY++;
            }
            if (GetAsyncKeyState(VK_LEFT) & 0x8001 || GetAsyncKeyState(VK_A) & 0x8001) {
                if(playerPosX > 0)
                    playerPosX--;
            }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8001 || GetAsyncKeyState(VK_D) & 0x8001) {
                if(playerPosX + 2 < SCREEN_WIDTH - 16)
                    playerPosX++;
            }
            lastInputTick = currentTick;
        }

        char msg[100];

        FillBuffer(buffer, DARK_GRAY);
        DrawSprite("Assets\\Sprites\\Glass.png", buffer, 16, 16, SCREEN_WIDTH / 2 - 8 - 16, SCREEN_HEIGHT / 2 - 8);
        DrawSprite("Assets\\Sprites\\Player.png", buffer, 16, 16, playerPosX, playerPosY);
        DrawSprite("Assets\\Sprites\\MissingTex.png", buffer, 16, 16, SCREEN_WIDTH / 2 - 8 + 16, SCREEN_HEIGHT / 2 - 8);

        sprintf_s(msg, sizeof(msg), "Mouse postion (%d / %d)", point.x / 4, point.y / 4);
        DrawString(msg, buffer, WHITE, 1, 1);
        sprintf_s(msg, sizeof(msg), "Player postion (%d / %d)", playerPosX, playerPosY);
        DrawString(msg, buffer, WHITE, 1, 9);

        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }

    // 동적으로 할당된 배열 해제
    free(buffer);
    SetCursorPosition(0, 0);
    return 0;
}