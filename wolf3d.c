/**
 * 命令行伪3D渲染器 - 完整版 (修复段错误)
 * 编译: gcc -O3 -o wolf3d wolf3d.c -lm
 * 运行: ./wolf3d
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

/* ---------- 配置常量 ---------- */
#define MAP_WIDTH      32
#define MAP_HEIGHT     32
#define SCREEN_WIDTH   80
#define SCREEN_HEIGHT  40
#define FOV            M_PI_4
#define RENDER_DEPTH   16

#define PLAYER_RADIUS  0.30f
#define MOVE_SPEED     5.0f
#define ROT_SPEED      2.5f
#define MOUSE_SENS     0.008f

#define MAX_FRAME_TIME 0.05f
#define SUB_STEPS      2

/* 菜单状态 */
typedef enum {
    MENU_MAIN,
    MENU_SETTINGS,
    MENU_GAMEPLAY,
    MENU_QUIT
} MenuState;

/* 设置选项 */
typedef struct {
    float mouseSensitivity;
    int   showFPS;
    int   showMinimap;
    int   showCrosshair;
    float volume;
} Settings;

static Settings settings = {
    .mouseSensitivity = 0.008f,
    .showFPS = 1,
    .showMinimap = 1,
    .showCrosshair = 1,
    .volume = 0.5f
};

/* 输入键位映射 */
typedef struct {
    int forward;
    int backward;
    int left;
    int right;
    int turnLeft;
    int turnRight;
    int quit;
    int menu;
} KeyBindings;

static KeyBindings keys = {
    'w', 's', 'a', 'd', 'q', 'e', 27, 'm'
};

/* ---------- 材质定义 ---------- */
static const char* texChars[] = {
    NULL,
    " .,:;!|#@",   /* 砖墙 */
    " .,:;oO0#",   /* 石墙 */
    " .,:!|\\M#"    /* 木墙 */
};

static const int texColors[] = { 0, 130, 245, 94 };
#define FLOOR_COLOR   235
#define CEIL_COLOR    251

/* ---------- 世界地图 ---------- */
static const int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

/* 玩家状态 */
static float posX = 8.5f, posY = 8.5f;  /* 起始位置移到安全区域 */
static float dirX = -1.0f, dirY = 0.0f;
static float planeX = 0.0f, planeY = 0.66f;

/* 输入系统 */
static int keyState[512] = {0};  /* 扩大数组 */
static int mouseFd = -1;
static struct termios orig_termios;

/* 渲染缓存 */
static float cachedCameraX[SCREEN_WIDTH];
static float cachedRayDirX[SCREEN_WIDTH];
static float cachedRayDirY[SCREEN_WIDTH];

static float wallDist[SCREEN_WIDTH];
static int   wallSide[SCREEN_WIDTH];
static float wallTexX[SCREEN_WIDTH];
static int   wallMat[SCREEN_WIDTH];
static int   wallDrawStart[SCREEN_WIDTH];
static int   wallDrawEnd[SCREEN_WIDTH];

/* 帧率统计 */
static float fps = 60.0f;
static int frameCount = 0;
static struct timeval lastFPS;

/* 游戏状态 */
static int inGame = 0;
static MenuState menuState = MENU_MAIN;
static int menuSelection = 0;

/* ---------- 辅助函数 ---------- */
void prepareTerminal() {
    printf("\033[?25l\033[2J\033[H");
    fflush(stdout);
}

void restoreTerminal() {
    printf("\033[?25h\n");
    fflush(stdout);
}

int isSolidCell(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return 1;
    return worldMap[y][x] != 0;
}

int checkCollision(float x, float y) {
    int minX = (int)(x - PLAYER_RADIUS);
    int maxX = (int)(x + PLAYER_RADIUS);
    int minY = (int)(y - PLAYER_RADIUS);
    int maxY = (int)(y + PLAYER_RADIUS);

    for (int yi = minY; yi <= maxY; yi++) {
        for (int xi = minX; xi <= maxX; xi++) {
            if (isSolidCell(xi, yi)) {
                float closestX = fmax(xi - 0.5f, fmin(x, xi + 0.5f));
                float closestY = fmax(yi - 0.5f, fmin(y, yi + 0.5f));
                float dx = x - closestX;
                float dy = y - closestY;
                if (dx * dx + dy * dy < PLAYER_RADIUS * PLAYER_RADIUS)
                    return 1;
            }
        }
    }
    return 0;
}

void tryMove(float dx, float dy, float *newX, float *newY) {
    float origX = *newX, origY = *newY;

    float testX = origX + dx;
    if (!checkCollision(testX, origY)) {
        *newX = testX;
    }

    float testY = origY + dy;
    if (!checkCollision(origX, testY)) {
        *newY = testY;
    }

    if (!checkCollision(origX + dx, origY + dy)) {
        *newX = origX + dx;
        *newY = origY + dy;
    }
}

/* ---------- 终端控制 ---------- */
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int keyPressed() {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void updateKeyState() {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return;

    if (c == '\033') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return;
        if (seq[0] == '[') {
            int code = 0;
            switch (seq[1]) {
                case 'A': code = 1000; break;
                case 'B': code = 1001; break;
                case 'C': code = 1002; break;
                case 'D': code = 1003; break;
            }
            if (code && code < 512) keyState[code] = 1;
        }
        return;
    }

    if ((unsigned char)c < 512) keyState[(unsigned char)c] = 1;
}

void clearKeyState() {
    memset(keyState, 0, sizeof(keyState));
}

void initMouse() {
    #ifdef __linux__
    mouseFd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    #endif
}

void updateMouse(float *rotDelta) {
    if (mouseFd < 0) return;

    signed char data[3];
    ssize_t n = read(mouseFd, data, sizeof(data));
    if (n == 3) {
        int dx = data[1];
        *rotDelta += dx * settings.mouseSensitivity;
    }
}

/* ---------- 菜单渲染 ---------- */
void renderMenu(char screenChar[SCREEN_HEIGHT][SCREEN_WIDTH],
                int  screenColor[SCREEN_HEIGHT][SCREEN_WIDTH]) {
    /* 清屏 */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            screenChar[y][x] = ' ';
            screenColor[y][x] = 0;
        }
    }

    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    if (menuState == MENU_MAIN) {
        const char *title = "=== WOLF3D CLI ===";
        const char *options[] = {"Start Game", "Settings", "Quit"};
        int optCount = 3;

        int titleX = centerX - strlen(title) / 2;
        for (int i = 0; title[i] && titleX + i < SCREEN_WIDTH; i++) {
            screenChar[centerY - 6][titleX + i] = title[i];
            screenColor[centerY - 6][titleX + i] = 220;
        }

        for (int i = 0; i < optCount; i++) {
            int x = centerX - strlen(options[i]) / 2;
            int y = centerY - 2 + i * 2;
            if (y >= 0 && y < SCREEN_HEIGHT) {
                if (menuSelection == i && x-2 >= 0) {
                    screenChar[y][x - 2] = '>';
                    screenColor[y][x - 2] = 46;
                }
                for (int j = 0; options[i][j] && x + j < SCREEN_WIDTH; j++) {
                    screenChar[y][x + j] = options[i][j];
                    screenColor[y][x + j] = (menuSelection == i) ? 46 : 255;
                }
            }
        }

        const char *controls = "Use W/S to navigate, ENTER to select";
        int ctrlX = centerX - strlen(controls) / 2;
        int ctrlY = centerY + 8;
        if (ctrlY < SCREEN_HEIGHT) {
            for (int i = 0; controls[i] && ctrlX + i < SCREEN_WIDTH; i++) {
                screenChar[ctrlY][ctrlX + i] = controls[i];
                screenColor[ctrlY][ctrlX + i] = 240;
            }
        }
    }
    else if (menuState == MENU_SETTINGS) {
        const char *title = "=== SETTINGS ===";
        char options[4][32];
        snprintf(options[0], 32, "Mouse Sensitivity: %.3f", settings.mouseSensitivity);
        snprintf(options[1], 32, "Show FPS: %s", settings.showFPS ? "ON" : "OFF");
        snprintf(options[2], 32, "Show Minimap: %s", settings.showMinimap ? "ON" : "OFF");
        snprintf(options[3], 32, "Show Crosshair: %s", settings.showCrosshair ? "ON" : "OFF");
        int optCount = 4;

        int titleX = centerX - strlen(title) / 2;
        for (int i = 0; title[i] && titleX + i < SCREEN_WIDTH; i++) {
            screenChar[centerY - 6][titleX + i] = title[i];
            screenColor[centerY - 6][titleX + i] = 220;
        }

        for (int i = 0; i < optCount; i++) {
            int x = centerX - strlen(options[i]) / 2;
            int y = centerY - 2 + i * 2;
            if (y >= 0 && y < SCREEN_HEIGHT) {
                if (menuSelection == i && x-2 >= 0) {
                    screenChar[y][x - 2] = '>';
                    screenColor[y][x - 2] = 46;
                }
                for (int j = 0; options[i][j] && x + j < SCREEN_WIDTH; j++) {
                    screenChar[y][x + j] = options[i][j];
                    screenColor[y][x + j] = (menuSelection == i) ? 46 : 255;
                }
            }
        }

        const char *controls = "W/S: Navigate, +/-: Adjust, ESC: Back";
        int ctrlX = centerX - strlen(controls) / 2;
        int ctrlY = centerY + 8;
        if (ctrlY < SCREEN_HEIGHT) {
            for (int i = 0; controls[i] && ctrlX + i < SCREEN_WIDTH; i++) {
                screenChar[ctrlY][ctrlX + i] = controls[i];
                screenColor[ctrlY][ctrlX + i] = 240;
            }
        }
    }
                }

                /* ---------- 迷你地图 ---------- */
                void drawMinimap(char screenChar[SCREEN_HEIGHT][SCREEN_WIDTH],
                                 int  screenColor[SCREEN_HEIGHT][SCREEN_WIDTH]) {
                    if (!settings.showMinimap) return;

                    int mapW = 20;
                    int mapH = 12;  /* 减小高度 */
                    int startX = SCREEN_WIDTH - mapW - 2;
                    int startY = 2;

                    if (startX < 0 || startY + mapH >= SCREEN_HEIGHT) return;

                    /* 边框 */
                    for (int i = 0; i <= mapW && startX + i < SCREEN_WIDTH; i++) {
                        if (startY >= 0 && startY < SCREEN_HEIGHT) {
                            screenChar[startY][startX + i] = '-';
                            screenColor[startY][startX + i] = 240;
                        }
                        if (startY + mapH >= 0 && startY + mapH < SCREEN_HEIGHT) {
                            screenChar[startY + mapH][startX + i] = '-';
                            screenColor[startY + mapH][startX + i] = 240;
                        }
                    }
                    for (int i = 0; i <= mapH && startY + i < SCREEN_HEIGHT; i++) {
                        if (startX >= 0 && startX < SCREEN_WIDTH) {
                            screenChar[startY + i][startX] = '|';
                            screenColor[startY + i][startX] = 240;
                        }
                        if (startX + mapW >= 0 && startX + mapW < SCREEN_WIDTH) {
                            screenChar[startY + i][startX + mapW] = '|';
                            screenColor[startY + i][startX + mapW] = 240;
                        }
                    }

                    /* 绘制地图 */
                    for (int y = 0; y < mapH; y++) {
                        for (int x = 0; x < mapW; x++) {
                            int mapX = (int)(posX - mapW/2 + x);
                            int mapY = (int)(posY - mapH/2 + y);
                            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                                int px = startX + x;
                                int py = startY + y;
                                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                                    if (worldMap[mapY][mapX] > 0) {
                                        screenChar[py][px] = '#';
                                        screenColor[py][px] = 130;
                                    } else {
                                        screenChar[py][px] = '.';
                                        screenColor[py][px] = 235;
                                    }
                                }
                            }
                        }
                    }

                    /* 绘制玩家位置 */
                    int playerMapX = mapW/2;
                    int playerMapY = mapH/2;
                    int px = startX + playerMapX;
                    int py = startY + playerMapY;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        screenChar[py][px] = '@';
                        screenColor[py][px] = 46;
                    }

                    /* 标题 */
                    const char *title = "MINIMAP";
                    for (int i = 0; title[i] && startX + 2 + i < SCREEN_WIDTH; i++) {
                        if (startY - 1 >= 0) {
                            screenChar[startY - 1][startX + 2 + i] = title[i];
                            screenColor[startY - 1][startX + 2 + i] = 220;
                        }
                    }
                                 }

                                 /* ---------- 渲染核心 ---------- */
                                 void precomputeRays() {
                                     for (int x = 0; x < SCREEN_WIDTH; x++) {
                                         cachedCameraX[x] = 2.0f * x / (float)SCREEN_WIDTH - 1.0f;
                                         cachedRayDirX[x] = dirX + planeX * cachedCameraX[x];
                                         cachedRayDirY[x] = dirY + planeY * cachedCameraX[x];
                                     }
                                 }

                                 void renderFrame(char screenChar[SCREEN_HEIGHT][SCREEN_WIDTH],
                                                  int  screenColor[SCREEN_HEIGHT][SCREEN_WIDTH]) {
                                     precomputeRays();

                                     /* 墙壁光线投射 */
                                     for (int x = 0; x < SCREEN_WIDTH; x++) {
                                         float rayDirX = cachedRayDirX[x];
                                         float rayDirY = cachedRayDirY[x];

                                         int mapX = (int)posX;
                                         int mapY = (int)posY;

                                         float deltaDistX = fabs(1.0f / rayDirX);
                                         float deltaDistY = fabs(1.0f / rayDirY);

                                         int stepX, stepY;
                                         float sideDistX, sideDistY;
                                         if (rayDirX < 0) {
                                             stepX = -1;
                                             sideDistX = (posX - mapX) * deltaDistX;
                                         } else {
                                             stepX = 1;
                                             sideDistX = (mapX + 1.0f - posX) * deltaDistX;
                                         }
                                         if (rayDirY < 0) {
                                             stepY = -1;
                                             sideDistY = (posY - mapY) * deltaDistY;
                                         } else {
                                             stepY = 1;
                                             sideDistY = (mapY + 1.0f - posY) * deltaDistY;
                                         }

                                         int hit = 0, side = 0;
                                         while (!hit) {
                                             if (sideDistX < sideDistY) {
                                                 sideDistX += deltaDistX;
                                                 mapX += stepX;
                                                 side = 0;
                                             } else {
                                                 sideDistY += deltaDistY;
                                                 mapY += stepY;
                                                 side = 1;
                                             }
                                             if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                                                 if (worldMap[mapY][mapX] > 0) hit = 1;
                                             } else {
                                                 hit = 1; /* 边界视为墙 */
                                             }
                                             if (fabs(mapX - posX) + fabs(mapY - posY) > RENDER_DEPTH) break;
                                         }

                                         float perpWallDist;
                                         if (side == 0)
                                             perpWallDist = (sideDistX - deltaDistX);
                                         else
                                             perpWallDist = (sideDistY - deltaDistY);
                                         if (perpWallDist < 0.01f) perpWallDist = 0.01f;

                                         float texX;
                                         if (side == 0)
                                             texX = posY + rayDirY * perpWallDist - floor(posY + rayDirY * perpWallDist);
                                         else
                                             texX = posX + rayDirX * perpWallDist - floor(posX + rayDirX * perpWallDist);

                                         wallDist[x] = perpWallDist;
                                         wallSide[x] = side;
                                         wallTexX[x] = texX;

                                         int mat = 1;
                                         if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                                             mat = worldMap[mapY][mapX];
                                         }
                                         if (mat < 1 || mat > 3) mat = 1;
                                         wallMat[x] = mat;

                                         int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
                                         if (lineHeight > SCREEN_HEIGHT) lineHeight = SCREEN_HEIGHT;
                                         int drawStart = (SCREEN_HEIGHT - lineHeight) / 2;
                                         if (drawStart < 0) drawStart = 0;
                                         int drawEnd = drawStart + lineHeight;
                                         if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
                                         wallDrawStart[x] = drawStart;
                                         wallDrawEnd[x] = drawEnd;
                                     }

                                     /* 填充像素 */
                                     for (int y = 0; y < SCREEN_HEIGHT; y++) {
                                         for (int x = 0; x < SCREEN_WIDTH; x++) {
                                             if (y >= wallDrawStart[x] && y <= wallDrawEnd[x]) {
                                                 int mat = wallMat[x];
                                                 int idx = (int)(wallTexX[x] * 7.9f);
                                                 if (idx < 0) idx = 0;
                                                 if (idx > 7) idx = 7;
                                                 screenChar[y][x] = texChars[mat][idx];
                                                 screenColor[y][x] = texColors[mat];
                                             } else if (y < wallDrawStart[x]) {
                                                 screenChar[y][x] = ' ';
                                                 screenColor[y][x] = CEIL_COLOR;
                                             } else {
                                                 screenChar[y][x] = '.';
                                                 screenColor[y][x] = FLOOR_COLOR;
                                             }
                                         }
                                     }

                                     /* 准星 */
                                     if (settings.showCrosshair) {
                                         int cx = SCREEN_WIDTH / 2;
                                         int cy = SCREEN_HEIGHT / 2;
                                         if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
                                             screenChar[cy][cx] = '+';
                                             screenColor[cy][cx] = 46;
                                             if (cy > 0) { screenChar[cy-1][cx] = '|'; screenColor[cy-1][cx] = 46; }
                                             if (cy < SCREEN_HEIGHT-1) { screenChar[cy+1][cx] = '|'; screenColor[cy+1][cx] = 46; }
                                             if (cx > 0) { screenChar[cy][cx-1] = '-'; screenColor[cy][cx-1] = 46; }
                                             if (cx < SCREEN_WIDTH-1) { screenChar[cy][cx+1] = '-'; screenColor[cy][cx+1] = 46; }
                                         }
                                     }

                                     /* 帧率显示 */
                                     if (settings.showFPS) {
                                         char fpsStr[16];
                                         snprintf(fpsStr, sizeof(fpsStr), "FPS: %.0f", fps);
                                         int len = strlen(fpsStr);
                                         for (int i = 0; i < len && SCREEN_WIDTH - len + i < SCREEN_WIDTH; i++) {
                                             screenChar[1][SCREEN_WIDTH - len + i] = fpsStr[i];
                                             screenColor[1][SCREEN_WIDTH - len + i] = 46;
                                         }
                                     }

                                     /* 迷你地图 */
                                     drawMinimap(screenChar, screenColor);

                                     /* HUD信息 */
                                     char info[64];
                                     snprintf(info, sizeof(info), "Pos:%.0f,%.0f", posX, posY);
                                     for (int i = 0; info[i] && i < SCREEN_WIDTH; i++) {
                                         screenChar[0][i] = info[i];
                                         screenColor[0][i] = 231;
                                     }

                                     char help[32] = "M:Menu ESC:Quit";
                                     for (int i = 0; help[i] && i < SCREEN_WIDTH; i++) {
                                         screenChar[SCREEN_HEIGHT-1][i] = help[i];
                                         screenColor[SCREEN_HEIGHT-1][i] = 240;
                                     }
                                                  }

                                                  void presentBuffer(char screenChar[SCREEN_HEIGHT][SCREEN_WIDTH],
                                                                     int  screenColor[SCREEN_HEIGHT][SCREEN_WIDTH]) {
                                                      printf("\033[H");
                                                      for (int y = 0; y < SCREEN_HEIGHT; y++) {
                                                          int lastColor = -1;
                                                          for (int x = 0; x < SCREEN_WIDTH; x++) {
                                                              if (screenColor[y][x] != lastColor) {
                                                                  printf("\033[38;5;%dm", screenColor[y][x]);
                                                                  lastColor = screenColor[y][x];
                                                              }
                                                              putchar(screenChar[y][x]);
                                                          }
                                                          if (y < SCREEN_HEIGHT - 1) putchar('\n');
                                                      }
                                                      fflush(stdout);
                                                                     }

                                                                     /* 更新帧率 */
                                                                     void updateFPS() {
                                                                         frameCount++;
                                                                         struct timeval now;
                                                                         gettimeofday(&now, NULL);
                                                                         float elapsed = (now.tv_sec - lastFPS.tv_sec) +
                                                                         (now.tv_usec - lastFPS.tv_usec) / 1000000.0f;
                                                                         if (elapsed >= 0.5f) {
                                                                             fps = frameCount / elapsed;
                                                                             frameCount = 0;
                                                                             lastFPS = now;
                                                                         }
                                                                     }

                                                                     /* ---------- 主循环 ---------- */
                                                                     int main() {
                                                                         enableRawMode();
                                                                         prepareTerminal();
                                                                         initMouse();

                                                                         char screenChar[SCREEN_HEIGHT][SCREEN_WIDTH];
                                                                         int  screenColor[SCREEN_HEIGHT][SCREEN_WIDTH];

                                                                         int running = 1;
                                                                         struct timespec lastTime, currentTime;
                                                                         clock_gettime(CLOCK_MONOTONIC, &lastTime);
                                                                         gettimeofday(&lastFPS, NULL);

                                                                         while (running) {
                                                                             clock_gettime(CLOCK_MONOTONIC, &currentTime);
                                                                             float deltaTime = (currentTime.tv_sec - lastTime.tv_sec) +
                                                                             (currentTime.tv_nsec - lastTime.tv_nsec) / 1e9f;
                                                                             if (deltaTime > MAX_FRAME_TIME) deltaTime = MAX_FRAME_TIME;
                                                                             lastTime = currentTime;

                                                                             /* 输入更新 */
                                                                             while (keyPressed()) {
                                                                                 updateKeyState();
                                                                             }

                                                                             if (!inGame) {
                                                                                 /* 菜单模式 */
                                                                                 if (keyState['w'] || keyState['W'] || keyState[1000]) {
                                                                                     menuSelection--;
                                                                                     if (menuState == MENU_MAIN && menuSelection < 0) menuSelection = 2;
                                                                                     if (menuState == MENU_SETTINGS && menuSelection < 0) menuSelection = 3;
                                                                                     usleep(150000);
                                                                                 }
                                                                                 if (keyState['s'] || keyState['S'] || keyState[1001]) {
                                                                                     menuSelection++;
                                                                                     if (menuState == MENU_MAIN && menuSelection > 2) menuSelection = 0;
                                                                                     if (menuState == MENU_SETTINGS && menuSelection > 3) menuSelection = 0;
                                                                                     usleep(150000);
                                                                                 }
                                                                                 if (keyState['+'] || keyState['=']) {
                                                                                     if (menuState == MENU_SETTINGS && menuSelection == 0) {
                                                                                         settings.mouseSensitivity += 0.001f;
                                                                                         if (settings.mouseSensitivity > 0.03f) settings.mouseSensitivity = 0.03f;
                                                                                     }
                                                                                     usleep(150000);
                                                                                 }
                                                                                 if (keyState['-'] || keyState['_']) {
                                                                                     if (menuState == MENU_SETTINGS && menuSelection == 0) {
                                                                                         settings.mouseSensitivity -= 0.001f;
                                                                                         if (settings.mouseSensitivity < 0.001f) settings.mouseSensitivity = 0.001f;
                                                                                     }
                                                                                     usleep(150000);
                                                                                 }
                                                                                 if (keyState[' '] || keyState[10]) {
                                                                                     if (menuState == MENU_MAIN) {
                                                                                         if (menuSelection == 0) {
                                                                                             inGame = 1;
                                                                                             menuSelection = 0;
                                                                                         } else if (menuSelection == 1) {
                                                                                             menuState = MENU_SETTINGS;
                                                                                             menuSelection = 0;
                                                                                         } else if (menuSelection == 2) {
                                                                                             running = 0;
                                                                                         }
                                                                                     } else if (menuState == MENU_SETTINGS) {
                                                                                         if (menuSelection == 1) settings.showFPS = !settings.showFPS;
                                                                                         else if (menuSelection == 2) settings.showMinimap = !settings.showMinimap;
                                                                                         else if (menuSelection == 3) settings.showCrosshair = !settings.showCrosshair;
                                                                                     }
                                                                                     usleep(150000);
                                                                                 }
                                                                                 if (keyState[27] || keyState['q'] || keyState['Q']) {
                                                                                     if (menuState == MENU_SETTINGS) {
                                                                                         menuState = MENU_MAIN;
                                                                                         menuSelection = 0;
                                                                                     } else {
                                                                                         running = 0;
                                                                                     }
                                                                                     usleep(150000);
                                                                                 }

                                                                                 renderMenu(screenChar, screenColor);
                                                                                 presentBuffer(screenChar, screenColor);
                                                                                 clearKeyState();
                                                                                 usleep(50000);
                                                                                 continue;
                                                                             }

                                                                             /* 游戏模式 */
                                                                             float mouseRot = 0;
                                                                             updateMouse(&mouseRot);

                                                                             float moveForward = 0, moveStrafe = 0, rotDelta = 0;
                                                                             if (keyState[keys.forward]) moveForward = 1;
                                                                             if (keyState[keys.backward]) moveForward = -1;
                                                                             if (keyState[keys.left]) moveStrafe = -1;
                                                                             if (keyState[keys.right]) moveStrafe = 1;
                                                                             if (keyState[keys.turnLeft]) rotDelta = ROT_SPEED * deltaTime;
                                                                             if (keyState[keys.turnRight]) rotDelta = -ROT_SPEED * deltaTime;
                                                                             if (keyState[keys.menu]) {
                                                                                 inGame = 0;
                                                                                 menuState = MENU_MAIN;
                                                                                 menuSelection = 0;
                                                                                 usleep(200000);
                                                                             }
                                                                             if (keyState[keys.quit]) running = 0;

                                                                             rotDelta += mouseRot;

                                                                             float moveStep = MOVE_SPEED * deltaTime / SUB_STEPS;
                                                                             float dx = (dirX * moveForward + (-dirY) * moveStrafe) * moveStep;
                                                                             float dy = (dirY * moveForward + (dirX) * moveStrafe) * moveStep;

                                                                             float newX = posX, newY = posY;
                                                                             for (int i = 0; i < SUB_STEPS; i++) {
                                                                                 tryMove(dx, dy, &newX, &newY);
                                                                             }
                                                                             posX = newX;
                                                                             posY = newY;

                                                                             if (rotDelta != 0) {
                                                                                 float oldDirX = dirX;
                                                                                 dirX = dirX * cos(rotDelta) - dirY * sin(rotDelta);
                                                                                 dirY = oldDirX * sin(rotDelta) + dirY * cos(rotDelta);
                                                                                 float oldPlaneX = planeX;
                                                                                 planeX = planeX * cos(rotDelta) - planeY * sin(rotDelta);
                                                                                 planeY = oldPlaneX * sin(rotDelta) + planeY * cos(rotDelta);
                                                                             }

                                                                             renderFrame(screenChar, screenColor);
                                                                             presentBuffer(screenChar, screenColor);

                                                                             clearKeyState();
                                                                             updateFPS();
                                                                             usleep(16000);
                                                                         }

                                                                         if (mouseFd >= 0) close(mouseFd);
                                                                         restoreTerminal();
                                                                         return 0;
                                                                     }
