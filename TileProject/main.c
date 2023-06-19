#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <fcntl.h>

// 인클루드 가드
#define HAVE_STRUCT_TIMESPEC

#include <pthread.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 144

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

INPUT_RECORD rec;
DWORD dwNOER;
HANDLE CIN = 0;

bool threadExitFlag = false;

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
    PLAYER,
    PLAYER_ATTACK,
    DAMAGE_ZONE,
    ENEMY,
};

enum ItemID {
    RWITEM_NONE,
};

enum GameState {
    GS_Exit = 0,
    GS_Game,
    GS_TitleMenu,
    GS_Info,
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
} Vector2;

typedef struct {
    int hp;
    int point;
    Vector2 position;
} Player;

typedef struct {
    int hp;
    int maxHP;
    int damage;
    enum ItemID dropItem;
    Vector2 position;
    bool isDamaged;
    bool isBlink;
    clock_t previousDamageCooldown;
    int damageCooldown;
    bool bActive;
    clock_t previousDamageBlink;
} Enemy;

typedef struct {
    Vector2 startPosition;
    Vector2 targetPosition;
    int distanceLimit;
    char* spriteAdress;
    int damage;
} Projectile;

int mouseX = 0;
int mouseY = 0;
int clickState = 0;

Vector2 Normalize(Vector2 vector) {
    float magnitude = Magnitude(vector);

    Vector2 normalizedVector;
    normalizedVector.x = vector.x / magnitude;
    normalizedVector.y = vector.y / magnitude;

    return normalizedVector;
}

float Magnitude(Vector2 vector) {
    return sqrt(vector.x * vector.x + vector.y * vector.y);
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

double getDirDistance(int mouseX, int mouseY, Vector2 direction) {
    int dx = mouseX - direction.x;
    int dy = mouseY - direction.y;

    return sqrt(dx * dx + dy * dy);
}

int FindCloestDirection(int mouseX, int mouseY, int playerX, int playerY) {
    Vector2 Directions[] = {
    {72, 0},
    {143, 72},
    {72, 143},
    {0, 72}
    };
    Vector2 cloestDirection = Directions[0];
    int cloestIndex = 0;
    double minDistance = getDirDistance(mouseX, mouseY, cloestDirection);
    for (int i = 1; i < sizeof(Directions) / sizeof(Directions[0]); i++)
    {
        double distance = getDirDistance(mouseX, mouseY, Directions[i]);
        if (distance < minDistance) {
            minDistance = distance;
            cloestDirection = Directions[i];
            cloestIndex = i;
        }
    }

    return cloestIndex;
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

void DrawPixelBySize(CHAR_INFO* buffer, Color color, int x, int y, int bufferWidth, int bufferHeight) {
    if (color.a < 255)
        return;

    DefineColor closestColor = FindClosestConsoleColor(color);

    if (x >= 0 && y >= 0 && x < bufferWidth && y < bufferHeight) {
        x *= 2;
        buffer[y * (bufferWidth * 2) + x].Char.UnicodeChar = L'█';
        buffer[y * (bufferWidth * 2) + (x + 1)].Char.UnicodeChar = L' ';
        buffer[y * (bufferWidth * 2) + x].Attributes = closestColor.colorTag;
        buffer[y * (bufferWidth * 2) + (x + 1)].Attributes = closestColor.colorTag;
    }
}

void DrawPixelWithoutLimit(CHAR_INFO* buffer, Color color, int width, int x, int y) {
    if (color.a < 255)
        return;

    DefineColor closestColor = FindClosestConsoleColor(color);

    if (x >= 0 && y >= 0) {
        x *= 2;
        buffer[y * (width * 2) + x].Char.UnicodeChar = L'█';
        buffer[y * (width * 2) + (x + 1)].Char.UnicodeChar = L' ';
        buffer[y * (width * 2) + x].Attributes = closestColor.colorTag;
        buffer[y * (width * 2) + (x + 1)].Attributes = closestColor.colorTag;
    }
}

void DrawPixelColiderBySize(enum ObjectID* buffer, enum ObjectID ID, int x, int y, int bufferWidth) {
    if (x >= 0 && y >= 0) {
        x *= 2;
        buffer[y * (bufferWidth * 2) + x] = ID;
        buffer[y * (bufferWidth * 2) + (x + 1)] = ID;
    }
}

enum ObjectID GetColiderObjectID(enum ObjectID* coliderBuffer, int x, int y, int bufferWidth) {
    if (x >= 0 && y > 0 && x < bufferWidth && y < 400) {
        x = x * 2;
        return coliderBuffer[y * (bufferWidth * 2) + x];
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

void DrawPixelConsoleColorBySize(CHAR_INFO* buffer, enum ConsoleColor color, int x, int y, int bufferWidth, int bufferHeight) {
    if (x >= 0 && y >= 0 && x < bufferWidth && y < bufferHeight) {
        x *= 2;
        buffer[y * (bufferWidth * 2) + x].Char.UnicodeChar = L'█';
        buffer[y * (bufferWidth * 2) + (x + 1)].Char.UnicodeChar = L' ';
        buffer[y * (bufferWidth * 2) + x].Attributes = color;
        buffer[y * (bufferWidth * 2) + (x + 1)].Attributes = color;
    }
}

bool GetCollisionBySprite(IplImage* image, enum ObjectID* coliderBuffer, enum ObejctID targetID, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    _x -= width / 2;
    _y -= height / 2;

    int x = 0;
    int y = 0;
    int idx = 0;
    
    bool isCollision = false;
    enum ObjectID currentID = AIR;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char A = image->imageData[idx + 3];

        if (A > 254) {
            currentID = GetColiderObjectID(coliderBuffer, x + _x, y + _y, screenWidth, screenHeight);
            if (currentID == targetID)
                isCollision = true;
            else if(currentID >= targetID && targetID == ENEMY)
                isCollision = true;
        }
    }
    return isCollision;
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

    _x -= width / 2;
    _y -= height / 2;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * x + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DrawPixel(buffer, color, x + _x, y + _y);
    }
    cvReleaseImage(&image);
    cvReleaseImage(&resizedImage);
}

void DrawSpriteByZero(const char* fileName, CHAR_INFO* buffer, int width, int height, int _x, int _y) {
    IplImage* image = cvLoadImage(fileName, CV_LOAD_IMAGE_UNCHANGED);

    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);

    if (!image) {
        cvSet(resizedImage, CV_RGB(255, 0, 255), NULL);
    }
    else {
        cvResize(image, resizedImage, CV_INTER_LINEAR);
    }

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * x + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DrawPixel(buffer, color, x + _x, y + _y);
    }
    cvReleaseImage(&image);
    cvReleaseImage(&resizedImage);
}

void DrawSpriteByZeroWithoutLimit(const char* fileName, CHAR_INFO* buffer, int width, int height, int _x, int _y) {
    IplImage* image = cvLoadImage(fileName, CV_LOAD_IMAGE_UNCHANGED);

    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);

    if (!image) {
        cvSet(resizedImage, CV_RGB(255, 0, 255), NULL);
    }
    else {
        cvResize(image, resizedImage, CV_INTER_LINEAR);
    }

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * x + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DrawPixelWithoutLimit(buffer, color, width, x + _x, y + _y);
    }
    cvReleaseImage(&image);
    cvReleaseImage(&resizedImage);
}

void ShootProjectile(CHAR_INFO* buffer, Projectile projectileData) {
    IplImage* image = cvLoadImage(projectileData.spriteAdress, CV_LOAD_IMAGE_UNCHANGED);

    int width = image->width;
    int height = image->height;

    IplImage* resizedImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);

    if (!image) {
        cvSet(resizedImage, CV_RGB(255, 0, 255), NULL);
    }
    else {
        cvResize(image, resizedImage, CV_INTER_LINEAR);
    }

    Vector2 targetPosition = projectileData.targetPosition;

    Vector2 startPosition = projectileData.startPosition;

    Vector2 distance = {targetPosition.x - startPosition.x, targetPosition.y - startPosition.y};

    Vector2 direction = Normalize(distance);

    float fDistance = Magnitude(distance);

    int x = 0;
    int y = 0;
    int idx = 0;
    float _x = 0;
    float _y = 0;

    clock_t currentTime = clock();
    clock_t previousTime = 0;

    for (int i = 0; i < projectileData.distanceLimit; i++)
    {
        currentTime = clock();
        if (currentTime - previousTime > 100) {
            for (int i = 0; i < width * height; i++)
            {
                x = i % width;
                y = i / width;

                idx = 4 * x + y * image->widthStep;

                unsigned char R = image->imageData[idx + 2];
                unsigned char G = image->imageData[idx + 1];
                unsigned char B = image->imageData[idx];
                unsigned char A = image->imageData[idx + 3];
                Color color = { R, G, B, A };

                DrawPixel(buffer, color, x + (int)_x, y + (int)_y);
            }
            _x += direction.x;
            _y += direction.y;
            previousTime = currentTime;
        }
    }
    cvReleaseImage(&image);
    cvReleaseImage(&resizedImage);
}

void DrawSpriteIPL(IplImage* image, CHAR_INFO* buffer, int _x, int _y) {
    int width = image->width;
    int height = image->height;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * x + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DrawPixel(buffer, color, x + _x, y + _y);
    }
}

void DrawSpriteSheet(IplImage* image, CHAR_INFO* buffer, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    _x -= width / 2;
    _y -= height / 2;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        if (A > 254)
            DrawPixelBySize(buffer, color, x + _x, y + _y, screenWidth, screenHeight);
    }
}

void DrawSpriteSheetConsoleColor(IplImage* image, CHAR_INFO* buffer, enum ConsoleColor color, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    _x -= width / 2;
    _y -= height / 2;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char A = image->imageData[idx + 3];

        if (A > 254)
            DrawPixelConsoleColorBySize(buffer, color, x + _x, y + _y, screenWidth, screenHeight);
    }
}

int DrawColiderBySprite(IplImage* image, enum ObjectID* coliderBuffer, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    _x -= width / 2;
    _y -= height / 2;

    enum ObjectID objectID;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DefineColor closestColor = FindClosestConsoleColor(color);

        if (closestColor.colorTag == GREEN)
            objectID = SOLID;
        else if (closestColor.colorTag == RED)
            objectID = DAMAGE_ZONE;
        else if (closestColor.colorTag == BLUE)
            objectID = PLAYER_ATTACK;
        else if (closestColor.colorTag == DARK_RED)
            objectID = ENEMY;
        else
            objectID = AIR;

        if (A > 254) {
            DrawPixelColiderBySize(coliderBuffer, objectID, x + _x, y + _y, screenWidth);
        }
    }
}

int DrawColiderBySpriteWithID(IplImage* image, enum ObjectID* coliderBuffer, enum ObjectID ID, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    _x -= width / 2;
    _y -= height / 2;

    enum ObjectID objectID = ID;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DefineColor closestColor = FindClosestConsoleColor(color);

        if (A > 254) {
            DrawPixelColiderBySize(coliderBuffer, objectID, x + _x, y + _y, screenWidth);
        }
    }
}

int DrawColiderBySpriteByZero(IplImage* image, enum ObjectID* coliderBuffer, int screenWidth, int screenHeight, int width, int height, int index, int _x, int _y) {
    if (!image)
        return;

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    enum ObjectID objectID;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;

        idx = 4 * (x + index) + y * image->widthStep;

        unsigned char R = image->imageData[idx + 2];
        unsigned char G = image->imageData[idx + 1];
        unsigned char B = image->imageData[idx];
        unsigned char A = image->imageData[idx + 3];
        Color color = { R, G, B, A };

        DefineColor closestColor = FindClosestConsoleColor(color);

        if (closestColor.colorTag == GREEN)
            objectID = SOLID;
        else if (closestColor.colorTag == RED)
            objectID = DAMAGE_ZONE;
        else if (closestColor.colorTag == BLUE)
            objectID = PLAYER_ATTACK;
        else if (closestColor.colorTag == DARK_RED)
            objectID = ENEMY;
        else
            objectID = AIR;

        if (A > 254) {
            DrawPixelColiderBySize(coliderBuffer, objectID, x + _x, y + _y, screenWidth);
        }
    }
}

void DrawColider(enum ObjectID* coliderBuffer, enum ObjectID objectID, int screenWidth, int screenHeight, int width, int height, int _x, int _y) {
    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;
        DrawPixelColiderBySize(coliderBuffer, objectID, x + _x, y + _y, screenWidth, screenHeight);
    }
}

void DrawTextSprite(IplImage* image, CHAR_INFO* buffer, enum ConsoleColor color, int index, int _x, int _y) {
    int width = 3;
    int height = 5;

    if (!image)
        return;    

    int spriteWidth = image->width;

    index *= width;

    if (index > spriteWidth - width)
        index = 0;

    int x = 0;
    int y = 0;
    int idx = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;
        
        idx = x + index + y * image->widthStep;

        unsigned char A = image->imageData[idx];

        if (A > 254)
            DrawPixelConsoleColor(buffer, color, x + _x, y + _y);
    }
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

void DrawChar(IplImage* image, char character, CHAR_INFO* buffer, enum ConsoleColor color, int _x, int _y) {
    switch (character)
    {
    case 'A':
        DrawTextSprite(image, buffer, color, 1, _x, _y);
        break;
    case 'B':
        DrawTextSprite(image, buffer, color, 2, _x, _y);
        break;
    case 'C':
        DrawTextSprite(image, buffer, color, 3, _x, _y);
        break;
    case 'D':
        DrawTextSprite(image, buffer, color, 4, _x, _y);
        break;
    case 'E':
        DrawTextSprite(image, buffer, color, 5, _x, _y);
        break;
    case 'F':
        DrawTextSprite(image, buffer, color, 6, _x, _y);
        break;
    case 'G':
        DrawTextSprite(image, buffer, color, 7, _x, _y);
        break;
    case 'H':
        DrawTextSprite(image, buffer, color, 8, _x, _y);
        break;
    case 'I':
        DrawTextSprite(image, buffer, color, 9, _x, _y);
        break;
    case 'J':
        DrawTextSprite(image, buffer, color, 10, _x, _y);
        break;
    case 'K':
        DrawTextSprite(image, buffer, color, 11, _x, _y);
        break;
    case 'L':
        DrawTextSprite(image, buffer, color, 12, _x, _y);
        break;
    case 'M':
        DrawTextSprite(image, buffer, color, 13, _x, _y);
        break;
    case 'N':
        DrawTextSprite(image, buffer, color, 14, _x, _y);
        break;
    case 'O':
        DrawTextSprite(image, buffer, color, 15, _x, _y);
        break;
    case 'P':
        DrawTextSprite(image, buffer, color, 16, _x, _y);
        break;
    case 'Q':
        DrawTextSprite(image, buffer, color, 17, _x, _y);
        break;
    case 'R':
        DrawTextSprite(image, buffer, color, 18, _x, _y);
        break;
    case 'S':
        DrawTextSprite(image, buffer, color, 19, _x, _y);
        break;
    case 'T':
        DrawTextSprite(image, buffer, color, 20, _x, _y);
        break;
    case 'U':
        DrawTextSprite(image, buffer, color, 21, _x, _y);
        break;
    case 'V':
        DrawTextSprite(image, buffer, color, 22, _x, _y);
        break;
    case 'W':
        DrawTextSprite(image, buffer, color, 23, _x, _y);
        break;
    case 'X':
        DrawTextSprite(image, buffer, color, 24, _x, _y);
        break;
    case 'Y':
        DrawTextSprite(image, buffer, color, 25, _x, _y);
        break;
    case 'Z':
        DrawTextSprite(image, buffer, color, 26, _x, _y);
        break;
    case '0':
        DrawTextSprite(image, buffer, color, 27, _x, _y);
        break;
    case '1':
        DrawTextSprite(image, buffer, color, 28, _x, _y);
        break;
    case '2':
        DrawTextSprite(image, buffer, color, 29, _x, _y);
        break;
    case '3':
        DrawTextSprite(image, buffer, color, 30, _x, _y);
        break;
    case '4':
        DrawTextSprite(image, buffer, color, 31, _x, _y);
        break;
    case '5':
        DrawTextSprite(image, buffer, color, 32, _x, _y);
        break;
    case '6':
        DrawTextSprite(image, buffer, color, 33, _x, _y);
        break;
    case '7':
        DrawTextSprite(image, buffer, color, 34, _x, _y);
        break;
    case '8':
        DrawTextSprite(image, buffer, color, 35, _x, _y);
        break;
    case '9':
        DrawTextSprite(image, buffer, color, 36, _x, _y);
        break;
    case '.':
        DrawTextSprite(image, buffer, color, 37, _x, _y);
        break;
    case ',':
        DrawTextSprite(image, buffer, color, 38, _x, _y);
        break;
    case '!':
        DrawTextSprite(image, buffer, color, 39, _x, _y);
        break;
    case '?':
        DrawTextSprite(image, buffer, color, 40, _x, _y);
        break;
    case ':':
        DrawTextSprite(image, buffer, color, 41, _x, _y);
        break;
    case ';':
        DrawTextSprite(image, buffer, color, 42, _x, _y);
        break;
    case '(':
        DrawTextSprite(image, buffer, color, 43, _x, _y);
        break;
    case ')':
        DrawTextSprite(image, buffer, color, 44, _x, _y);
        break;
    case '[':
        DrawTextSprite(image, buffer, color, 45, _x, _y);
        break;
    case ']':
        DrawTextSprite(image, buffer, color, 46, _x, _y);
        break;
    case '<':
        DrawTextSprite(image, buffer, color, 47, _x, _y);
        break;
    case '>':
        DrawTextSprite(image, buffer, color, 48, _x, _y);
        break;
    case '-':
        DrawTextSprite(image, buffer, color, 49, _x, _y);
        break;
    case '+':
        DrawTextSprite(image, buffer, color, 50, _x, _y);
        break;
    case '=':
        DrawTextSprite(image, buffer, color, 51, _x, _y);
        break;
    case '|':
        DrawTextSprite(image, buffer, color, 52, _x, _y);
        break;
    case '_':
        DrawTextSprite(image, buffer, color, 53, _x, _y);
        break;
    case '%':
        DrawTextSprite(image, buffer, color, 54, _x, _y);
        break;
    case '/':
        DrawTextSprite(image, buffer, color, 55, _x, _y);
        break;
    case '\'':
        DrawTextSprite(image, buffer, color, 56, _x, _y);
        break;
    case '\"':
        DrawTextSprite(image, buffer, color, 57, _x, _y);
        break;
    case '@':
        DrawTextSprite(image, buffer, color, 58, _x, _y);
        break;
    case '^':
        DrawTextSprite(image, buffer, color, 59, _x, _y);
        break;
    case '~':
        DrawTextSprite(image, buffer, color, 60, _x, _y);
        break;
    case ' ':
        break;
    default:
        DrawTextSprite(image, buffer, color, 0, _x, _y);
        break;
    }
}

void DrawString(const char* string, CHAR_INFO* buffer, enum ConsoleColor color, int _x, int _y) {
    IplImage* image = cvLoadImage("Assets\\Sprites\\Font.png", 0);
    for (int i = 0; i < strlen(string); i++)
    {
        DrawChar(image, string[i], buffer, color, _x + (i * 4), _y);
    }
    cvReleaseImage(&image);
}

void DrawBox(CHAR_INFO* buffer, enum ConsoleColor color, int width, int height, int _x, int _y) {
    int x = 0;
    int y = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;
        DrawPixelConsoleColor(buffer, color, x + _x, y + _y);
    }
}

void DrawBoxBySize(CHAR_INFO* buffer, enum ConsoleColor color, int width, int height, int _x, int _y, int screenWidth, int screenHeight) {
    int x = 0;
    int y = 0;
    for (int i = 0; i < width * height; i++)
    {
        x = i % width;
        y = i / width;
        DrawPixelConsoleColorBySize(buffer, color, x + _x, y + _y, screenWidth, screenHeight);
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

void SetConsoleWindowSize(int columns, int rows) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT rect = { 0, 0, columns - 1, rows - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &rect);
}

void Init() {
    SetFontSize(6);
    HWND hWnd = GetConsoleWindow(); // 현재 콘솔 창의 핸들을 가져옵니다.
    HANDLE consoleInputHandle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE consoleOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD consoleModePrev;
    setlocale(LC_ALL, ""); // 언어 설정을 현재 컴퓨터의 언어설정으로 변경

    SMALL_RECT windowRect = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT };  // 예시로 가로 80, 세로 30의 크기로 설정
    COORD bufferSize = { SCREEN_WIDTH * 2 , SCREEN_HEIGHT + 1 };  // 버퍼 크기와 콘솔 창 크기를 동일하게 설정
    SetConsoleWindowInfo(consoleOutputHandle, TRUE, &windowRect);
    SetConsoleScreenBufferSize(consoleOutputHandle, bufferSize);

    _setmode(_fileno(stdout), _O_U16TEXT); // 기본 변환 모드를 유니코드로 설정
    wchar_t Title[50] = L"RUNEWORLD";
    SetConsoleTitleW(Title);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
    GetConsoleMode(consoleInputHandle, &consoleModePrev);
    SetConsoleMode(consoleInputHandle, consoleModePrev & ~ENABLE_QUICK_EDIT_MODE);

    // 수직 스크롤바 숨기기
    CONSOLE_SCREEN_BUFFER_INFOEX infoEx;
    infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    GetConsoleScreenBufferInfoEx(consoleOutputHandle, &infoEx);
    infoEx.dwSize.Y = bufferSize.Y - 1;  // 버퍼의 세로 크기를 콘솔 창의 세로 크기와 동일하게 설정
    SetConsoleScreenBufferInfoEx(consoleOutputHandle, &infoEx);
}

void* GetMouseStateThread(void* arg) {
    clock_t previous = clock();
    bool isClicked = false;
    while (true)
    {
        clock_t current = clock();
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &dwNOER); // 콘솔창 입력을 받아들임.
        if (rec.EventType == MOUSE_EVENT) {// 마우스 이벤트일 경우
            if (rec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) { // 좌측 버튼이 클릭되었을 경우
                clickState = 1; //마우스 클릭상태를 넘김
                isClicked = true;
                previous = current;
            }
            else if (rec.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) { // 우측 버튼이 클릭되었을 경우
                clickState = 2; //마우스 클릭상태를 넘김
                isClicked = true;
                previous = current;
            }


            mouseX = (rec.Event.MouseEvent.dwMousePosition.X / 2); // X값 받아옴
            mouseY = rec.Event.MouseEvent.dwMousePosition.Y; // Y값 받아옴
        }
        if (current - previous > 1) {
            clickState = 0;
            previous = current;
        }
    }

    pthread_exit(NULL);
}

enum GameState TitleMenu() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정

    IplImage* titleImage = cvLoadImage("Assets\\Sprites\\Title.png", CV_LOAD_IMAGE_UNCHANGED);

    clock_t previous = clock();
    clock_t lag = 0;

    DrawSpriteIPL(titleImage, buffer, 0, 0);

    char msg[150];

    enum GameState stateBeRetrun = GS_TitleMenu;

    clickState = 0;
    while (true)
    {
        clock_t current = clock();

        if (current - previous > 1000 / 30) {

            DrawSpriteIPL(titleImage, buffer, 0, 0);

            char msg[60];
            sprintf_s(msg, sizeof(msg), "(%d, %d) %d", mouseX, mouseY, clickState);
            DrawString(msg, buffer, WHITE, 0, 0);

            if (mouseX > 52 && mouseX < 91 && mouseY > 54 && mouseY < 70 && clickState == 1) {
                stateBeRetrun = GS_Game;
                break;
            }
            else if (mouseX > 52 && mouseX < 91 && mouseY > 54 && mouseY < 70) {
                DrawSprite("Assets\\Sprites\\TitleSelectBox.png", buffer, 40, 16, 72, 62);
            }

            if (mouseX > 52 && mouseX < 91 && mouseY > 74 && mouseY < 90 && clickState == 1) {
                stateBeRetrun = GS_Info;
                break;
            }
            else if (mouseX > 52 && mouseX < 91 && mouseY > 74 && mouseY < 90) {
                DrawSprite("Assets\\Sprites\\TitleSelectBox.png", buffer, 40, 16, 72, 83);
            }

            if (mouseX > 52 && mouseX < 91 && mouseY > 94 && mouseY < 110 && clickState == 1) {
                stateBeRetrun = GS_Exit;
                break;
            }
            else if (mouseX > 53 && mouseX < 91 && mouseY > 94 && mouseY < 110) {
                DrawSprite("Assets\\Sprites\\TitleSelectBox.png", buffer, 40, 16, 72, 104);
            }

            previous = current;
        }
        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }

    // 동적으로 할당된 배열 해제
    free(buffer);
    cvReleaseImage(&titleImage);
    return stateBeRetrun;
}

enum GameState InfoMenu() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정

    IplImage* titleImage = cvLoadImage("Assets\\Sprites\\InfoMenu.png", CV_LOAD_IMAGE_UNCHANGED);

    clock_t previous = clock();
    clock_t lag = 0;

    DrawSpriteIPL(titleImage, buffer, 0, 0);

    clickState = 0;
    while (true)
    {
        clock_t current = clock();

        if (current - previous > 1000 / 30) {

            DrawSpriteIPL(titleImage, buffer, 0, 0);

            char msg[60];
            sprintf_s(msg, sizeof(msg), "(%d, %d) %d", mouseX, mouseY, clickState);
            DrawString(msg, buffer, WHITE, 0, 0);

            DrawString("RUNEWORLD IS HARDCORE RETRO RPG", buffer, WHITE, 9, 33);
            DrawString("PLAY IN COMMAND PROMPT.", buffer, WHITE, 9, 33 + 6);

            DrawString("ARROW KEY: MOVEMENT", buffer, WHITE, 9, 33 + (6 * 2) + 2);
            DrawString("LEFT CLICK: MELEE ATTACK", buffer, WHITE, 9, 33 + (6 * 3) + 2);
            DrawString("RIGHT CLICK: RANGED ATTACK", buffer, WHITE, 9, 33 + (6 * 4) + 2);
            DrawString("E KEY: INVENTORY", buffer, WHITE, 9, 33 + (6 * 5) + 2);
            DrawString("ESC KEY: PAUSE MENU", buffer, WHITE, 9, 33 + (6 * 6) + 2);

            DrawString("DEVELOPED BY KIM DONG HYEON", buffer, WHITE, SCREEN_WIDTH / 2 - (27 * 4 / 2), 100);

            if (mouseX > 41 && mouseX < 103 && mouseY > 117 && mouseY < 132 && clickState == 1) {
                DrawString("PLAY CLICKED!", buffer, WHITE, 0, 10);
                break;
            }
            else if (mouseX > 41 && mouseX < 103 && mouseY > 117 && mouseY < 132) {
                DrawSprite("Assets\\Sprites\\SelectBox63x16.png", buffer, 63, 16, 72, 125);
            }

            previous = current;
        }
        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }

    // 동적으로 할당된 배열 해제
    free(buffer);
    cvReleaseImage(&titleImage);
    return GS_TitleMenu;
}

enum GameState Game() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // 표준 출력 핸들 가져오기

    CHAR_INFO* mapBuffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * 400 * 2 * 400);
    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);
    enum ObjectID* coliderBuffer = (enum ObjectID*)malloc(sizeof(enum ObjectID) * 400 * 2 * 400);

    int slashSize = 16;
    IplImage* originalSlashSprite = cvLoadImage("Assets\\Sprites\\SlashSprite.png", CV_LOAD_IMAGE_UNCHANGED);
    IplImage* originalSlashColider = cvLoadImage("Assets\\Sprites\\SlashColider.png", CV_LOAD_IMAGE_UNCHANGED);
    
    IplImage* slashSprite = cvCreateImage(cvSize(slashSize * 4, slashSize), IPL_DEPTH_8U, 4);
    IplImage* slashColider = cvCreateImage(cvSize(slashSize * 4, slashSize), IPL_DEPTH_8U, 4);

    IplImage* mapColider = cvLoadImage("Assets\\Sprites\\MapColider.png", CV_LOAD_IMAGE_UNCHANGED);
    IplImage* playerSprite = cvLoadImage("Assets\\Sprites\\PlayerSheet.png", CV_LOAD_IMAGE_UNCHANGED);
    IplImage* playerColider = cvLoadImage("Assets\\Sprites\\PlayerColider.png", CV_LOAD_IMAGE_UNCHANGED);
    IplImage* enemySprite = cvLoadImage("Assets\\Sprites\\EnemySheet.png", CV_LOAD_IMAGE_UNCHANGED);
    IplImage* enemyrColider = cvLoadImage("Assets\\Sprites\\EnemyColider.png", CV_LOAD_IMAGE_UNCHANGED);

    COORD bufferSize = { SCREEN_WIDTH * 2, SCREEN_HEIGHT };  // 버퍼 크기 설정
    COORD bufferCoord = { 0, 0 };  // 출력 위치 설정
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH * 2 - 1, SCREEN_HEIGHT - 1 };  // 출력 영역 설정

    clock_t previous = clock();
    clock_t previousInput = clock();
    clock_t previousAnimation = clock();
    clock_t previousDamageCooldown = clock();
    clock_t previousDamageBlink = clock();
    clock_t previousAttackCooldown = clock();
    clock_t previousAttackDuration = clock();

    int attackDuration = 0;
    const int maxAttackDuration = 2;


    bool isBlink = false;

    bool isDamaged = false;
    int damageCooldown = 0;
    const int maxDamageCooldown = 2;

    char msg[100];

    bool Pause = false;
    bool isStateChange = true;
    bool isLeft = false;

    int animationIdx = 0;

    Vector2 previousPlayerPosition = { 0, 0 };
    Vector2 screenMovement = { 0, 0 };
    
    const int barLEN = 92;
    float barTick = (float)100/barLEN;
    const int maxHP = 100;
    const int maxMoney = 100;
    Player player = { maxHP, 5, {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2} };

    int slashDamage = 1;

    Vector2 playerDirection = { 0, 0 };

    int enemyCount = 30;

    Enemy enemy[30];
    for (int i = 0; i < enemyCount; i++)
    {
        enemy[i].hp = 50;
        enemy[i].maxHP = 50;
        enemy[i].damage = 10;
        enemy[i].dropItem = RWITEM_NONE;
        enemy[i].position.x = rand() % 382 + 1;
        enemy[i].position.y = rand() % 382 + 1;
        DrawColiderBySpriteWithID(enemyrColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, 16, 16, 0, enemy[i].position.x, enemy[i].position.y);
        if(GetCollisionBySprite(playerColider, coliderBuffer, ENEMY, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y)) {
            enemy[i].position.x = rand() % 382 + 1;
            enemy[i].position.y = rand() % 382 + 1;
        }
        enemy[i].isDamaged = false;
        enemy[i].isBlink = false;
        enemy[i].previousDamageCooldown = clock();
        enemy[i].damageCooldown = 0;
        enemy[i].bActive = true;
        enemy[i].previousDamageBlink = clock();
    }

    Vector2 current = { 0, 0 };
    Vector2 destination = { 100, 50 };

    int attackDirection = 0;

    int _lastIndex = 0;

    bool isAttack = false;
    bool isPlayerMove = true;

    enum GameState stateBeRetrun = GS_Game;
    DrawSpriteByZeroWithoutLimit("Assets\\Sprites\\Map.png", mapBuffer, 400, 400, 0, 0);

    clickState = 0;
    while (true)
    {
        if (player.hp <= 0) {
            while (true)
            {
                DrawSprite("Assets\\Sprites\\YouDied.png", buffer, 144, 144, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
                WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
            }
        }
        if (player.point >= maxMoney) {
            while (true)
            {
                DrawSprite("Assets\\Sprites\\YouWin.png", buffer, 144, 144, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
                WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
            }
        }
        slashSprite = cvCreateImage(cvSize(slashSize * 4, slashSize), IPL_DEPTH_8U, 4);
        slashColider = cvCreateImage(cvSize(slashSize * 4, slashSize), IPL_DEPTH_8U, 4);

        cvResize(originalSlashSprite, slashSprite, CV_INTER_LINEAR);
        cvResize(originalSlashColider, slashColider, CV_INTER_LINEAR);

        clock_t current = clock();

        if (current - previousInput > 30) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                Pause = true;
            }
            bool previousIsLeft = isLeft;
            bool previousIsMove = isPlayerMove;
            if (GetAsyncKeyState(VK_W) & 0x8001 || GetAsyncKeyState(VK_S) & 0x8001 || GetAsyncKeyState(VK_A) & 0x8001 || GetAsyncKeyState(VK_D) & 0x8001) {

                previousPlayerPosition.y = player.position.y;
                previousPlayerPosition.x = player.position.x;
                if (GetAsyncKeyState(VK_W) & 0x8001) {
                    if(player.position.y > 0 + 8)
                        player.position.y -= 1;
                }
                if (GetAsyncKeyState(VK_S) & 0x8001) {
                    if (player.position.y < 400 - 1)
                        player.position.y += 1;
                }
                if (GetAsyncKeyState(VK_A) & 0x8001) {
                    if (player.position.x > 0 + 6)
                        player.position.x -= 1;
                    isLeft = true;
                }
                if (GetAsyncKeyState(VK_D) & 0x8001) {
                    if (player.position.x < 400 - 1)
                        player.position.x += 1;
                    isLeft = false;
                }
                isPlayerMove = true;
            }
            else {
                isPlayerMove = false;
            }
            if (clickState == 1) {
                isAttack = true;
                attackDirection = FindCloestDirection(mouseX, mouseY, player.position.x, player.position.y);
            }
            if (previousIsLeft != isLeft || previousIsMove != isPlayerMove)
                isStateChange = true;
            else {
                isStateChange = false;
            }
            previousInput = current;
        }
        bool isRunning = true;
        if (current - previous > 1000 / 30 && isRunning) {
            if (Pause){
                while (true) {
                    FillBuffer(buffer, BLACK);
                    DrawSprite("Assets\\Sprites\\PauseMenu.png", buffer, 61, 65, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
                    if (mouseX > 42 && mouseX < 102 && mouseY > 40 && mouseY < 53 && clickState == 1) {
                        Pause = false;
                        break;
                    }
                    else if (mouseX > 42 && mouseX < 102 && mouseY > 40 && mouseY < 53) {
                        DrawSprite("Assets\\Sprites\\SelectBox63x16.png", buffer, 63, 16, 72, 47);
                    }

                    if (mouseX > 42 && mouseX < 102 && mouseY > 56 && mouseY < 70 && clickState == 1) {
                        while (true)
                        {
                            FillBuffer(buffer, BLACK); 
                            DrawSprite("Assets\\Sprites\\Shop.png", buffer, 144, 144, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);


                            DrawString("4 GOLD", buffer, WHITE, 85, 9);
                            if (mouseX > 4 && mouseX < 83 && mouseY > 9 && mouseY < 23 && clickState == 1) {
                                clickState = 0;
                                if (player.point >= 4) {
                                    player.point -= 4;
                                    slashSize += 2;
                                }
                            }
                            else if (mouseX > 4 && mouseX < 83 && mouseY > 9 && mouseY < 23) {
                                DrawSprite("Assets\\Sprites\\SelectBox80x16.png", buffer, 80, 16, 43, 16);
                            }
                            DrawString("4 GOLD", buffer, WHITE, 85, 26);
                            if (mouseX > 4 && mouseX < 83 && mouseY > 26 && mouseY < 39 && clickState == 1) {
                                clickState = 0;
                                if (player.point >= 4) {
                                    player.point -= 4;
                                    slashDamage += 2;
                                }
                            }
                            else if (mouseX > 4 && mouseX < 83 && mouseY > 26 && mouseY < 39) {
                                DrawSprite("Assets\\Sprites\\SelectBox80x16.png", buffer, 80, 16, 43, 33);
                            }
                            DrawString("2 GOLD", buffer, WHITE, 51, 43);
                            if (mouseX > 4 && mouseX < 48 && mouseY > 43 && mouseY < 56 && clickState == 1) {
                                clickState = 0;
                                if (player.point >= 2 && player.hp < maxHP) {
                                    player.hp += 1;
                                    player.point -= 2;
                                }
                            }
                            else if (mouseX > 4 && mouseX < 48 && mouseY > 43 && mouseY < 56) {
                                DrawSprite("Assets\\Sprites\\SelectBox46x16.png", buffer, 46, 16, 27, 50);
                            }

                            if (mouseX > 3 && mouseX < 45 && mouseY > 124 && mouseY < 139 && clickState == 1) {
                                break;
                            }
                            else if (mouseX > 3 && mouseX < 45 && mouseY > 124 && mouseY < 139) {
                                DrawSprite("Assets\\Sprites\\SelectBox43x16.png", buffer, 43, 16, 24, 132);
                            }

                            sprintf_s(msg, sizeof(msg), "GOLD : % d", player.point);
                            DrawString(msg, buffer, WHITE, 6, 70);
                            sprintf_s(msg, sizeof(msg), "HP : %d", player.hp);
                            DrawString(msg, buffer, WHITE, 6, 77);

                            sprintf_s(msg, sizeof(msg), "DRILL SIZE : % d", slashSize);
                            DrawString(msg, buffer, WHITE, 65, 70);
                            sprintf_s(msg, sizeof(msg), "DRILL DAMAGE : %d", slashDamage);
                            DrawString(msg, buffer, WHITE, 65, 77);

                            WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
                        }
                    }
                    else if (mouseX > 42 && mouseX < 102 && mouseY > 56 && mouseY < 70) {
                        DrawSprite("Assets\\Sprites\\SelectBox63x16.png", buffer, 63, 16, 72, 64);
                    }

                    if (mouseX > 42 && mouseX < 102 && mouseY > 91 && mouseY < 104 && clickState == 1) {
                        return GS_TitleMenu;
                    }
                    else if (mouseX > 42 && mouseX < 102 && mouseY > 91 && mouseY < 104) {
                        DrawSprite("Assets\\Sprites\\SelectBox63x16.png", buffer, 63, 16, 72, 98);
                    }
                    WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
                }
            }


            DrawSpriteByZeroWithoutLimit("Assets\\Sprites\\Map.png", mapBuffer, 400, 400, 0, 0);

            DrawColider(coliderBuffer, AIR, 400, 400, 400, 400, 0, 0);

            DrawBoxBySize(mapBuffer, YELLOW, 40, 40, 20, 20, 400, 400);
            DrawColider(coliderBuffer, SOLID, 400, 400, 40, 40, 20, 20);

            if (GetCollisionBySprite(playerColider, coliderBuffer, SOLID, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y)) {
                player.position.x = previousPlayerPosition.x;
                player.position.y = previousPlayerPosition.y;
            }


            for (int i = 0; i < enemyCount; i++)
            {
                if (enemy[i].bActive == true) {
                    DrawSpriteSheet(enemySprite, mapBuffer, 400, 400, 16, 16, 0, enemy[i].position.x, enemy[i].position.y);
                    DrawColiderBySpriteWithID(enemyrColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, 16, 16, 0, enemy[i].position.x, enemy[i].position.y);
                }
            }
            
            if (isAttack) {
                if (current - previousAttackDuration > 200) {
                    attackDuration++;
                    if (attackDuration >= maxAttackDuration) {
                        isAttack = false;
                    }
                    previousAttackDuration = current;
                }
                if (attackDirection == 0) {
                    DrawSpriteSheet(slashSprite, mapBuffer, 400, 400, slashSize, slashSize, attackDirection, player.position.x, player.position.y - 13);
                    for (int i = 0; i < enemyCount; i++)
                    {
                        if (GetCollisionBySprite(slashColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, slashSize, slashSize, attackDirection, player.position.x, player.position.y - 13) && enemy[i].isDamaged == false) {
                            if (enemy[i].bActive == true) {
                                if (enemy[i].hp > 0)
                                    enemy[i].hp -= slashDamage;
                                if (enemy[i].hp <= 0) {
                                    enemy[i].hp = enemy[i].maxHP;
                                    enemy[i].position.x = rand() % 382 + 1;
                                    enemy[i].position.y = rand() % 382 + 1;
                                    if (slashSize < 150)
                                        slashSize += 2;
                                    player.point++;
                                }
                                enemy[i].isDamaged = true;
                            }
                        }
                    }
                }
                else if (attackDirection == 1) {
                    DrawSpriteSheet(slashSprite, mapBuffer, 400, 400, slashSize, slashSize, attackDirection, player.position.x + 13, player.position.y);
                    for (int i = 0; i < enemyCount; i++)
                    {
                        if (GetCollisionBySprite(slashColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, slashSize, slashSize, attackDirection, player.position.x + 13, player.position.y) && enemy[i].isDamaged == false) {
                            if (enemy[i].bActive == true) {
                                if (enemy[i].hp > 0)
                                    enemy[i].hp -= slashDamage;
                                if (enemy[i].hp <= 0) {
                                    enemy[i].hp = enemy[i].maxHP;
                                    enemy[i].position.x = rand() % 382 + 1;
                                    enemy[i].position.y = rand() % 382 + 1;
                                    if (slashSize < 150)
                                        slashSize += 2;
                                    player.point++;
                                }
                                enemy[i].isDamaged = true;
                            }
                        }
                    }
                }
                else if (attackDirection == 2) {
                    DrawSpriteSheet(slashSprite, mapBuffer, 400, 400, slashSize, slashSize, attackDirection, player.position.x, player.position.y + 13);
                    for (int i = 0; i < enemyCount; i++)
                    {
                        if (GetCollisionBySprite(slashColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, slashSize, slashSize, attackDirection, player.position.x, player.position.y + 13) && enemy[i].isDamaged == false) {
                            if (enemy[i].bActive == true) {
                                if (enemy[i].hp > 0)
                                    enemy[i].hp -= slashDamage;
                                if (enemy[i].hp <= 0) {
                                    enemy[i].hp = enemy[i].maxHP;
                                    enemy[i].position.x = rand() % 382 + 1;
                                    enemy[i].position.y = rand() % 382 + 1;
                                    if (slashSize < 150)
                                        slashSize += 2;
                                    player.point++;
                                }
                                enemy[i].isDamaged = true;
                            }
                        }
                    }
                }
                else if (attackDirection == 3) {
                    DrawSpriteSheet(slashSprite, mapBuffer, 400, 400, slashSize, slashSize, attackDirection, player.position.x - 13, player.position.y);
                    for (int i = 0; i < enemyCount; i++)
                    {
                        if (GetCollisionBySprite(slashColider, coliderBuffer, (ENEMY + 1) + i, 400, 400, slashSize, slashSize, attackDirection, player.position.x - 13, player.position.y) && enemy[i].isDamaged == false) {
                            if (enemy[i].bActive == true) {
                                if (enemy[i].hp > 0)
                                    enemy[i].hp -= slashDamage;
                                if (enemy[i].hp <= 0) {
                                    enemy[i].hp = enemy[i].maxHP;
                                    enemy[i].position.x = rand() % 382 + 1;
                                    enemy[i].position.y = rand() % 382 + 1;
                                    if (slashSize < 150)
                                        slashSize += 2;
                                    player.point++;
                                }
                                enemy[i].isDamaged = true;
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < enemyCount; i++)
            {
                if (enemy[i].bActive) {
                    float tick = 100 / 16;
                    float percent = (float)enemy[i].hp / enemy[i].maxHP * 100;
                    int barWidth = percent / tick;
                    DrawBoxBySize(mapBuffer, GREEN, barWidth, 2, enemy[i].position.x - 16 / 2, enemy[i].position.y + 9, 400, 400);
                    sprintf_s(msg, sizeof(msg), "%d", enemy[i].hp);
                }
            }

            if (isLeft) {
                if (isPlayerMove) {
                    if (current - previousAnimation > 100 || isStateChange) {
                        animationIdx++;
                        if (animationIdx > 15 || animationIdx < 10) {
                            animationIdx = 10;
                        }
                        previousAnimation = current;
                    }
                }
                else {
                    if (current - previousAnimation > 300 || isStateChange) {
                        animationIdx++;
                        if (animationIdx > 9 || animationIdx < 8) {
                            animationIdx = 8;
                        }
                        previousAnimation = current;
                    }
                }
            }
            else {
                if (isPlayerMove) {
                    if (current - previousAnimation > 100 || isStateChange) {
                        animationIdx++;
                        if (animationIdx > 7) {
                            animationIdx = 2;
                        }
                        previousAnimation = current;
                    }
                }
                else {
                    if (current - previousAnimation > 300 || isStateChange) {
                        animationIdx++;
                        if (animationIdx > 1) {
                            animationIdx = 0;
                        }
                        previousAnimation = current;
                    }
                }
            }
            if (isDamaged) {
                if (current - previousDamageCooldown > 1000) {
                    damageCooldown++;
                    if (damageCooldown >= maxDamageCooldown) {
                        damageCooldown = 0;
                        isDamaged = false;
                        isBlink = false;
                    }
                    previousDamageCooldown = current;
                }
                if (current - previousDamageBlink > 100) {
                    isBlink = !isBlink;
                    previousDamageBlink = current;
                }
                if (isBlink) {
                    DrawSpriteSheetConsoleColor(playerSprite, mapBuffer, WHITE, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y);
                }
                else {
                    DrawSpriteSheet(playerSprite, mapBuffer, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y);
                }
            }
            else {
                DrawSpriteSheet(playerSprite, mapBuffer, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y);
            }

            for (int i = 0; i < enemyCount; i++)
            {
                if (enemy[i].bActive == true) {
                    if (enemy[i].isDamaged) {
                        if (current - enemy[i].previousDamageCooldown > 500) {
                            enemy[i].damageCooldown++;
                            if (enemy[i].damageCooldown >= maxDamageCooldown) {
                                enemy[i].damageCooldown = 0;
                                enemy[i].isDamaged = false;
                                enemy[i].isBlink = false;
                            }
                            enemy[i].previousDamageCooldown = current;
                        }
                        if (current - enemy[i].previousDamageBlink > 100) {
                            enemy[i].isBlink = !enemy[i].isBlink;
                            enemy[i].previousDamageBlink = current;
                        }
                        if (enemy[i].isBlink) {
                            DrawSpriteSheetConsoleColor(enemySprite, mapBuffer, WHITE, 400, 400, 16, 16, 0, enemy[i].position.x, enemy[i].position.y);
                        }
                        else {
                            DrawSpriteSheet(enemySprite, mapBuffer, 400, 400, 16, 16, 0, enemy[i].position.x, enemy[i].position.y);
                        }
                    }
                }
            }

            if (GetCollisionBySprite(playerColider, coliderBuffer, ENEMY, 400, 400, 16, 16, animationIdx, player.position.x, player.position.y) && !isDamaged) {
                if(player.hp > 0)
                    player.hp -= 3;
                isDamaged = true;
                previousDamageCooldown = current;
            }
                

            if(player.position.x > SCREEN_WIDTH / 2 - 1 && player.position.x < 400 - SCREEN_WIDTH / 2 + 16 / 2 - 7)
                screenMovement.x = player.position.x * 2 - (SCREEN_WIDTH / 2 * 2);
            if(player.position.y > SCREEN_HEIGHT / 2 - 1 && player.position.y < 400 - SCREEN_HEIGHT / 2 + 16 / 2 - 7)
                screenMovement.y = player.position.y - (SCREEN_HEIGHT / 2);

            for (int y = 0; y < SCREEN_HEIGHT; y++)
            {
                for (int x = 0; x < SCREEN_WIDTH * 2; x++)
                {
                    buffer[y * SCREEN_WIDTH * 2 + x] = mapBuffer[(y + (int)screenMovement.y) * 400 * 2 + (x + (int)screenMovement.x)];
                }
            }
            DrawSprite("Assets\\Sprites\\BarUI.png", buffer, 120, 17, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 9);
            
            float percent = (float)player.hp / maxHP * 100;
            int barWidth = percent / barTick;

            DrawBox(buffer, RED, barWidth, 2, 26, 136);

            percent = (float)player.point / maxMoney * 100;
            int gBarWidth = percent / barTick;
            DrawBox(buffer, YELLOW, gBarWidth, 2, 26, 140);

            sprintf_s(msg, sizeof(msg), "(%d, %d) %d", mouseX, mouseY, clickState);
            DrawString(msg, buffer, WHITE, 0, 0);
            sprintf_s(msg, sizeof(msg), "GOLD : %d", player.point);
            DrawString(msg, buffer, WHITE, 0, 8);

            _lastIndex++;
            previous = current;
        }

        WriteConsoleOutputW(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
    }

    // 동적으로 할당된 배열 해제
    free(buffer);
    free(mapBuffer);
    free(coliderBuffer);
    free(enemy);
    cvReleaseImage(&originalSlashSprite);
    cvReleaseImage(&slashSprite);
    cvReleaseImage(&playerSprite);
    cvReleaseImage(&playerColider);
    cvReleaseImage(&enemySprite);
    return stateBeRetrun;
}

int	main() {
    Init();

    pthread_t thread;
    pthread_create(&thread, NULL, GetMouseStateThread, NULL);

    CHAR_INFO* buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_WIDTH * 2 * SCREEN_HEIGHT);

    enum GameState gameState = GS_TitleMenu;
    PlayVideo("Assets\\Title.mp4", buffer, 144, 144, 0, 0);
    while (1)
    {
        switch (gameState)
        {
        case GS_Exit:
            return 0;
        case GS_Game:
            gameState = Game();
            break;
        case GS_TitleMenu:
            gameState = TitleMenu();
            break;
        case GS_Info:
            gameState = InfoMenu();
            break;
        default:
            return 0;
        }
    }

    pthread_join(thread, NULL);

    SetCursorPosition(0, 0);
    return 0;
}