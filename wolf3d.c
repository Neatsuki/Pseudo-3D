// =============================
// Wolf3D CLI - Part 1
// 基础引擎层
// =============================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <fcntl.h>

// =============================
// 配置
// =============================

#define W 80
#define H 40

#define MAP_W 24
#define MAP_H 24

#define MOVE_SPEED 5.0f
#define ROT_SPEED  2.5f

// =============================
// 数据结构
// =============================

// 玩家
typedef struct {
    float x, y;
    float dirX, dirY;
    float planeX, planeY;
    int hp;
} Player;

// 输入
static int key[512];

// 地图
static int worldMap[MAP_W][MAP_H];

// 终端状态
static struct termios orig_termios;

// 时间
static struct timeval lastTime;

// 玩家实例
static Player player;

// =============================
// 数学工具
// =============================

static float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

// =============================
// 终端控制
// =============================

static void disableRaw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enableRaw() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRaw);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// 清屏
static void clearScreen() {
    printf("\033[2J\033[H");
}

// =============================
// 输入系统
// =============================

static int kbhit() {
    struct timeval tv = {0, 0};
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

static void updateInput() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        key[(unsigned char)c] = 1;
    }
}

static void clearInput() {
    memset(key, 0, sizeof(key));
}

// =============================
// 时间系统
// =============================

static float getDeltaTime() {
    struct timeval now;
    gettimeofday(&now, NULL);

    float dt =
        (now.tv_sec - lastTime.tv_sec) +
        (now.tv_usec - lastTime.tv_usec) / 1000000.0f;

    lastTime = now;
    return dt;
}

// =============================
// 地图系统
// =============================

// 简单地图
static void initMap() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (y == 0 || x == 0 || y == MAP_H - 1 || x == MAP_W - 1)
                worldMap[x][y] = 1;
            else
                worldMap[x][y] = 0;
        }
    }

    worldMap[10][10] = 1;
    worldMap[11][10] = 1;
    worldMap[12][10] = 1;
}

// 是否是墙
static int isWall(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H)
        return 1;
    return worldMap[x][y] != 0;
}

// =============================
// 碰撞系统
// =============================

static int checkCollision(float x, float y) {
    int ix = (int)x;
    int iy = (int)y;

    return isWall(ix, iy);
}

// =============================
// 玩家系统
// =============================

static void initPlayer() {
    player.x = 3.5f;
    player.y = 3.5f;

    player.dirX = -1.0f;
    player.dirY = 0.0f;

    player.planeX = 0.0f;
    player.planeY = 0.66f;

    player.hp = 100;
}

static void movePlayer(float dt) {
    float move = 0;

    if (key['w']) move = 1;
    if (key['s']) move = -1;

    float newX = player.x + player.dirX * move * MOVE_SPEED * dt;
    float newY = player.y + player.dirY * move * MOVE_SPEED * dt;

    if (!checkCollision(newX, player.y))
        player.x = newX;

    if (!checkCollision(player.x, newY))
        player.y = newY;
}

static void rotatePlayer(float dt) {
    float rot = 0;

    if (key['a']) rot = 1;
    if (key['d']) rot = -1;

    float angle = rot * ROT_SPEED * dt;

    float oldDirX = player.dirX;
    player.dirX = player.dirX * cosf(angle) - player.dirY * sinf(angle);
    player.dirY = oldDirX * sinf(angle) + player.dirY * cosf(angle);

    float oldPlaneX = player.planeX;
    player.planeX = player.planeX * cosf(angle) - player.planeY * sinf(angle);
    player.planeY = oldPlaneX * sinf(angle) + player.planeY * cosf(angle);
}

// =============================
// 初始化
// =============================

static void initEngine() {
    memset(key, 0, sizeof(key));

    enableRaw();
    clearScreen();

    initMap();
    initPlayer();

    gettimeofday(&lastTime, NULL);
}

// =============================
// 调试输出
// =============================

static void drawDebug() {
    printf("\033[H");
    printf("Wolf3D Part1\n");
    printf("Player: %.2f %.2f\n", player.x, player.y);
}
// =============================
// Wolf3D CLI - Part 2
// 射线投射 + 渲染核心
// =============================

#include <stdio.h>
#include <math.h>

// =============================
// 渲染缓冲区
// =============================

static char screen[W][H];
static float zbuffer[W];

// =============================
// 清空屏幕缓冲
// =============================

static void clearBuffer() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            screen[x][y] = ' ';
        }
    }
}

// =============================
// DDA射线投射
// =============================

static float castRay(float rayDirX, float rayDirY, int *hitSide) {
    int mapX = (int)player.x;
    int mapY = (int)player.y;

    float deltaDistX = fabsf(1.0f / (rayDirX == 0 ? 0.0001f : rayDirX));
    float deltaDistY = fabsf(1.0f / (rayDirY == 0 ? 0.0001f : rayDirY));

    int stepX, stepY;
    float sideDistX, sideDistY;

    if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (player.x - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0f - player.x) * deltaDistX;
    }

    if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (player.y - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0f - player.y) * deltaDistY;
    }

    int hit = 0;
    int side = 0;

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

        if (isWall(mapX, mapY))
            hit = 1;
    }

    *hitSide = side;

    float dist;
    if (side == 0)
        dist = sideDistX - deltaDistX;
    else
        dist = sideDistY - deltaDistY;

    return dist;
}

// =============================
// 渲染一帧（墙）
// =============================

static void renderWalls() {
    for (int x = 0; x < W; x++) {

        float cameraX = 2.0f * x / (float)W - 1.0f;

        float rayDirX = player.dirX + player.planeX * cameraX;
        float rayDirY = player.dirY + player.planeY * cameraX;

        int side;
        float dist = castRay(rayDirX, rayDirY, &side);

        zbuffer[x] = dist;

        int lineHeight = (int)(H / (dist + 0.0001f));

        int drawStart = -lineHeight / 2 + H / 2;
        int drawEnd   = lineHeight / 2 + H / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= H) drawEnd = H - 1;

        char wallChar = side ? '|' : '#';

        for (int y = 0; y < H; y++) {

            if (y >= drawStart && y <= drawEnd) {
                screen[x][y] = wallChar;
            }
        }
    }
}

// =============================
// 地板+天花板
// =============================

static void renderFloorCeil() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {

            if (screen[x][y] == ' ') {

                if (y < H / 2)
                    screen[x][y] = '.';
                else
                    screen[x][y] = ',';
            }
        }
    }
}

// =============================
// 输出渲染
// =============================

static void present() {
    printf("\033[H");

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            putchar(screen[x][y]);
        }
        putchar('\n');
    }
}
// =============================
// Wolf3D CLI - Part 3
// 实体 + 游戏逻辑
// =============================

#define MAX_ENEMY 32
#define MAX_BULLET 64
#define MAX_DOOR 32

// =============================
// 实体系统
// =============================

typedef struct {
    float x, y;
    int hp;
    int alive;
} Enemy;

typedef struct {
    float x, y;
    float dx, dy;
    int active;
} Bullet;

typedef struct {
    int x, y;
    int open; // 0关 1开
} Door;

// =============================
// 世界实体
// =============================

static Enemy enemies[MAX_ENEMY];
static Bullet bullets[MAX_BULLET];
static Door doors[MAX_DOOR];

// =============================
// 初始化实体
// =============================

static void initEntities() {

    for (int i = 0; i < MAX_ENEMY; i++) {
        enemies[i].x = 5 + i;
        enemies[i].y = 5;
        enemies[i].hp = 3;
        enemies[i].alive = 1;
    }

    for (int i = 0; i < MAX_BULLET; i++) {
        bullets[i].active = 0;
    }

    for (int i = 0; i < MAX_DOOR; i++) {
        doors[i].x = 10;
        doors[i].y = 10 + i;
        doors[i].open = 0;
    }
}

// =============================
// 子弹系统
// =============================

static void spawnBullet() {
    for (int i = 0; i < MAX_BULLET; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player.x;
            bullets[i].y = player.y;
            bullets[i].dx = player.dirX;
            bullets[i].dy = player.dirY;
            bullets[i].active = 1;
            break;
        }
    }
}

static void updateBullets(float dt) {

    for (int i = 0; i < MAX_BULLET; i++) {
        if (!bullets[i].active) continue;

        bullets[i].x += bullets[i].dx * 10.0f * dt;
        bullets[i].y += bullets[i].dy * 10.0f * dt;

        int mx = (int)bullets[i].x;
        int my = (int)bullets[i].y;

        if (isWall(mx, my)) {
            bullets[i].active = 0;
        }
    }
}

// =============================
// 敌人AI（简单追踪）
// =============================

static void updateEnemies(float dt) {

    for (int i = 0; i < MAX_ENEMY; i++) {

        if (!enemies[i].alive) continue;

        float dx = player.x - enemies[i].x;
        float dy = player.y - enemies[i].y;

        float len = sqrtf(dx*dx + dy*dy);
        if (len < 0.01f) continue;

        dx /= len;
        dy /= len;

        enemies[i].x += dx * dt * 1.5f;
        enemies[i].y += dy * dt * 1.5f;

        // 碰撞墙
        if (isWall((int)enemies[i].x, (int)enemies[i].y)) {
            enemies[i].x -= dx * dt * 1.5f;
            enemies[i].y -= dy * dt * 1.5f;
        }
    }
}

// =============================
// 子弹击中敌人
// =============================

static void bulletHitTest() {

    for (int b = 0; b < MAX_BULLET; b++) {
        if (!bullets[b].active) continue;

        for (int e = 0; e < MAX_ENEMY; e++) {
            if (!enemies[e].alive) continue;

            float dx = bullets[b].x - enemies[e].x;
            float dy = bullets[b].y - enemies[e].y;

            if (dx*dx + dy*dy < 0.3f) {
                enemies[e].hp--;
                bullets[b].active = 0;

                if (enemies[e].hp <= 0)
                    enemies[e].alive = 0;
            }
        }
    }
}

// =============================
// 门系统
// =============================

static void openDoor(int x, int y) {

    for (int i = 0; i < MAX_DOOR; i++) {
        if (doors[i].x == x && doors[i].y == y) {
            doors[i].open = 1;
            worldMap[x][y] = 0;
        }
    }
}

// =============================
// 玩家射击
// =============================

static void updateCombat() {

    if (key[' ']) {
        spawnBullet();
        key[' '] = 0;
    }
}

// =============================
// 更新所有游戏逻辑
// =============================

static void updateGame(float dt) {

    movePlayer(dt);
    rotatePlayer(dt);

    updateBullets(dt);
    updateEnemies(dt);

    bulletHitTest();
    updateCombat();
}
// =============================
// Wolf3D CLI - Part 4
// 渲染整合 + HUD + 主循环
// =============================

// =============================
// HUD状态
// =============================

static int ammo = 30;
static int score = 0;

// =============================
// 精灵渲染（敌人）
// =============================

static void renderEnemies() {

    for (int i = 0; i < MAX_ENEMY; i++) {

        if (!enemies[i].alive) continue;

        float dx = enemies[i].x - player.x;
        float dy = enemies[i].y - player.y;

        float invDet = 1.0f /
            (player.planeX * player.dirY - player.dirX * player.planeY);

        float transformX =
            invDet * (player.dirY * dx - player.dirX * dy);

        float transformY =
            invDet * (-player.planeY * dx + player.planeX * dy);

        if (transformY <= 0) continue;

        int screenX = (int)((W / 2) * (1 + transformX / transformY));

        int size = (int)(H / transformY);

        int startY = -size / 2 + H / 2;
        int endY   = size / 2 + H / 2;

        int startX = screenX - size / 2;
        int endX   = screenX + size / 2;

        for (int x = startX; x < endX; x++) {

            if (x < 0 || x >= W) continue;
            if (transformY >= zbuffer[x]) continue;

            for (int y = startY; y < endY; y++) {

                if (y < 0 || y >= H) continue;

                screen[x][y] = 'E';
            }
        }
    }
}

// =============================
// HUD绘制
// =============================

static void renderHUD() {

    char line[128];

    snprintf(line, sizeof(line),
        "HP:%d  Ammo:%d  Score:%d  Pos:%.1f %.1f",
        player.hp, ammo, score, player.x, player.y
    );

    for (int i = 0; i < W && line[i]; i++) {
        screen[i][H - 1] = line[i];
    }
}

// =============================
// 输入控制整合
// =============================

static void handleInput(float dt) {

    if (key['x']) {
        openDoor((int)player.x, (int)player.y);
        key['x'] = 0;
    }

    if (key['r']) {
        ammo = 30;
        key['r'] = 0;
    }

    if (key[27]) { // ESC
        disableRaw();
        clearScreen();
        exit(0);
    }
}

// =============================
// 完整渲染流程
// =============================

static void renderFrame() {

    clearBuffer();

    renderWalls();
    renderFloorCeil();

    renderEnemies();
    renderHUD();

    present();
}

// =============================
// 主循环
// =============================

int main() {

    initEngine();
    initEntities();

    while (1) {

        float dt = getDeltaTime();

        updateInput();

        handleInput(dt);
        updateGame(dt);

        renderFrame();

        clearInput();

        usleep(16000);
    }

    return 0;
}
// =============================
// Wolf3D CLI - Part 5
// 游戏增强 + 优化层
// =============================

// =============================
// 拾取物
// =============================

typedef struct {
    float x, y;
    int type; // 0血包 1弹药
    int active;
} Pickup;

#define MAX_PICKUP 32

static Pickup pickups[MAX_PICKUP];

// =============================
// 初始化拾取物
// =============================

static void initPickups() {

    for (int i = 0; i < MAX_PICKUP; i++) {
        pickups[i].x = 6 + i * 0.5f;
        pickups[i].y = 6;
        pickups[i].type = i % 2;
        pickups[i].active = 1;
    }
}

// =============================
// 拾取检测
// =============================

static void updatePickups() {

    for (int i = 0; i < MAX_PICKUP; i++) {

        if (!pickups[i].active) continue;

        float dx = pickups[i].x - player.x;
        float dy = pickups[i].y - player.y;

        if (dx*dx + dy*dy < 0.5f) {

            if (pickups[i].type == 0)
                player.hp += 20;
            else
                ammo += 10;

            if (player.hp > 100) player.hp = 100;

            pickups[i].active = 0;
        }
    }
}

// =============================
// 改进敌人AI（状态机）
// =============================

typedef enum {
    AI_IDLE,
    AI_CHASE,
    AI_ATTACK
} AIState;

static AIState enemyState[MAX_ENEMY];

static void updateEnemyAI(float dt) {

    for (int i = 0; i < MAX_ENEMY; i++) {

        if (!enemies[i].alive) continue;

        float dx = player.x - enemies[i].x;
        float dy = player.y - enemies[i].y;

        float dist2 = dx*dx + dy*dy;

        if (dist2 < 1.0f)
            enemyState[i] = AI_ATTACK;
        else if (dist2 < 25.0f)
            enemyState[i] = AI_CHASE;
        else
            enemyState[i] = AI_IDLE;

        if (enemyState[i] == AI_CHASE) {

            float len = sqrtf(dist2) + 0.0001f;

            enemies[i].x += dx / len * dt * 1.2f;
            enemies[i].y += dy / len * dt * 1.2f;
        }

        if (enemyState[i] == AI_ATTACK) {

            player.hp -= 1;

            if (player.hp < 0)
                player.hp = 0;
        }
    }
}

// =============================
// 输入优化（防抖）
// =============================

static void handleBetterInput(float dt) {

    static float shootCooldown = 0;

    shootCooldown -= dt;

    if (key[' ']) {
        if (shootCooldown <= 0) {
            spawnBullet();
            ammo--;
            if (ammo < 0) ammo = 0;
            shootCooldown = 0.3f;
        }
        key[' '] = 0;
    }

    if (key['h']) {
        player.hp += 10;
        if (player.hp > 100) player.hp = 100;
        key['h'] = 0;
    }
}

// =============================
// 调试HUD
// =============================

static void renderDebug() {

    char buf[64];

    snprintf(buf, sizeof(buf),
        "Enemies:%d Bullets:%d",
        MAX_ENEMY, MAX_BULLET
    );

    for (int i = 0; i < W && buf[i]; i++) {
        screen[i][0] = buf[i];
    }
}

// =============================
// 渲染增强版
// =============================

static void renderFrameV2() {

    clearBuffer();

    renderWalls();
    renderFloorCeil();

    renderEnemies();
    updatePickups();

    renderHUD();
    renderDebug();

    present();
}

// =============================
// 更新增强版
// =============================

static void updateGameV2(float dt) {

    movePlayer(dt);
    rotatePlayer(dt);

    updateBullets(dt);
    updateEnemyAI(dt);
    updatePickups();

    bulletHitTest();

    handleBetterInput(dt);
    handleInput(dt);
}
// =============================
// Wolf3D CLI - Part 6
// 渲染增强 + 世界扩展
// =============================

// =============================
// 距离雾效字符
// =============================

static char shadeChar(float d) {
    if (d < 1.5f) return '#';
    if (d < 3.0f) return 'O';
    if (d < 5.0f) return 'o';
    if (d < 8.0f) return '.';
    return ' ';
}

// =============================
// 改进墙体渲染（带雾）
// =============================

static void renderWallsV2() {

    for (int x = 0; x < W; x++) {

        float cameraX = 2.0f * x / (float)W - 1.0f;

        float rayDirX = player.dirX + player.planeX * cameraX;
        float rayDirY = player.dirY + player.planeY * cameraX;

        int side;
        float dist = castRay(rayDirX, rayDirY, &side);

        zbuffer[x] = dist;

        int lineHeight = (int)(H / (dist + 0.0001f));

        int drawStart = -lineHeight / 2 + H / 2;
        int drawEnd   = lineHeight / 2 + H / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= H) drawEnd = H - 1;

        char c = shadeChar(dist);

        for (int y = drawStart; y <= drawEnd; y++) {
            screen[x][y] = c;
        }
    }
}

// =============================
// 拾取物渲染
// =============================

static void renderPickups() {

    for (int i = 0; i < MAX_PICKUP; i++) {

        if (!pickups[i].active) continue;

        float dx = pickups[i].x - player.x;
        float dy = pickups[i].y - player.y;

        float invDet =
            1.0f / (player.planeX * player.dirY - player.dirX * player.planeY);

        float transformX =
            invDet * (player.dirY * dx - player.dirX * dy);

        float transformY =
            invDet * (-player.planeY * dx + player.planeX * dy);

        if (transformY <= 0) continue;

        int screenX = (int)((W / 2) * (1 + transformX / transformY));

        int size = (int)(H / transformY);

        for (int y = -size/2 + H/2; y < size/2 + H/2; y++) {

            if (y < 0 || y >= H) continue;
            if (screenX < 0 || screenX >= W) continue;
            if (transformY >= zbuffer[screenX]) continue;

            screen[screenX][y] = (pickups[i].type == 0) ? '+' : '*';
        }
    }
}
// =============================
// Wolf3D CLI - Part 6
// 渲染增强 + 世界扩展
// =============================

// =============================
// 距离雾效字符
// =============================

static char shadeChar(float d) {
    if (d < 1.5f) return '#';
    if (d < 3.0f) return 'O';
    if (d < 5.0f) return 'o';
    if (d < 8.0f) return '.';
    return ' ';
}

// =============================
// 改进墙体渲染（带雾）
// =============================

static void renderWallsV2() {

    for (int x = 0; x < W; x++) {

        float cameraX = 2.0f * x / (float)W - 1.0f;

        float rayDirX = player.dirX + player.planeX * cameraX;
        float rayDirY = player.dirY + player.planeY * cameraX;

        int side;
        float dist = castRay(rayDirX, rayDirY, &side);

        zbuffer[x] = dist;

        int lineHeight = (int)(H / (dist + 0.0001f));

        int drawStart = -lineHeight / 2 + H / 2;
        int drawEnd   = lineHeight / 2 + H / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= H) drawEnd = H - 1;

        char c = shadeChar(dist);

        for (int y = drawStart; y <= drawEnd; y++) {
            screen[x][y] = c;
        }
    }
}

// =============================
// 拾取物渲染
// =============================

static void renderPickups() {

    for (int i = 0; i < MAX_PICKUP; i++) {

        if (!pickups[i].active) continue;

        float dx = pickups[i].x - player.x;
        float dy = pickups[i].y - player.y;

        float invDet =
            1.0f / (player.planeX * player.dirY - player.dirX * player.planeY);

        float transformX =
            invDet * (player.dirY * dx - player.dirX * dy);

        float transformY =
            invDet * (-player.planeY * dx + player.planeX * dy);

        if (transformY <= 0) continue;

        int screenX = (int)((W / 2) * (1 + transformX / transformY));

        int size = (int)(H / transformY);

        for (int y = -size/2 + H/2; y < size/2 + H/2; y++) {

            if (y < 0 || y >= H) continue;
            if (screenX < 0 || screenX >= W) continue;
            if (transformY >= zbuffer[screenX]) continue;

            screen[screenX][y] = (pickups[i].type == 0) ? '+' : '*';
        }
    }
}
// =============================
// Wolf3D CLI - Part 8
// MAX升级层（重构优化版）
// =============================

#include <time.h>

// =============================
// 武器系统（重构）
// =============================

typedef enum {
    WEAPON_PISTOL,
    WEAPON_SHOTGUN
} WeaponType;

static WeaponType currentWeapon = WEAPON_PISTOL;

static float shootCD = 0.0f;

static void switchWeapon() {
    if (key['1']) currentWeapon = WEAPON_PISTOL;
    if (key['2']) currentWeapon = WEAPON_SHOTGUN;
}

// =============================
// 子弹结构优化（穿透+伤害）
// =============================

typedef struct {
    float x, y, dx, dy;
    int active;
    int damage;
    int pierce;
} BulletV2;

static BulletV2 bulletsV2[MAX_BULLET];

// =============================
// 初始化子弹系统
// =============================

static void initBulletsV2() {
    for (int i = 0; i < MAX_BULLET; i++)
        bulletsV2[i].active = 0;
}

// =============================
// 发射子弹（统一接口）
// =============================

static void spawnBulletV2(float dx, float dy, int dmg) {
    for (int i = 0; i < MAX_BULLET; i++) {
        if (!bulletsV2[i].active) {
            bulletsV2[i].x = player.x;
            bulletsV2[i].y = player.y;
            bulletsV2[i].dx = dx;
            bulletsV2[i].dy = dy;
            bulletsV2[i].damage = dmg;
            bulletsV2[i].pierce = 1;
            bulletsV2[i].active = 1;
            break;
        }
    }
}

// =============================
// 霰弹枪（真实扩散）
// =============================

static void shotgunFire() {
    for (int i = 0; i < 7; i++) {
        float a = ((rand() % 100) / 100.0f - 0.5f) * 0.6f;

        float dx = player.dirX * cosf(a) - player.dirY * sinf(a);
        float dy = player.dirX * sinf(a) + player.dirY * cosf(a);

        spawnBulletV2(dx, dy, 1);
    }
}

// =============================
// 射击控制（冷却+武器）
// =============================

static void fireWeapon(float dt) {
    shootCD -= dt;
    if (!key[' '] || shootCD > 0) return;

    if (ammo <= 0) return;

    if (currentWeapon == WEAPON_PISTOL) {
        spawnBulletV2(player.dirX, player.dirY, 1);
        ammo--;
        shootCD = 0.25f;
    } else {
        shotgunFire();
        ammo--;
        shootCD = 0.6f;
    }

    key[' '] = 0;
}

// =============================
// 子弹更新（穿透+伤害）
// =============================

static void updateBulletsV2(float dt) {
    for (int i = 0; i < MAX_BULLET; i++) {
        if (!bulletsV2[i].active) continue;

        bulletsV2[i].x += bulletsV2[i].dx * 12.0f * dt;
        bulletsV2[i].y += bulletsV2[i].dy * 12.0f * dt;

        int mx = (int)bulletsV2[i].x;
        int my = (int)bulletsV2[i].y;

        if (isWall(mx, my)) {
            bulletsV2[i].active = 0;
            continue;
        }

        for (int e = 0; e < MAX_ENEMY; e++) {
            if (!enemies[e].alive) continue;

            float dx = bulletsV2[i].x - enemies[e].x;
            float dy = bulletsV2[i].y - enemies[e].y;

            if (dx*dx + dy*dy < 0.25f) {
                enemies[e].hp -= bulletsV2[i].damage;

                if (enemies[e].hp <= 0)
                    enemies[e].alive = 0;

                if (--bulletsV2[i].pierce <= 0)
                    bulletsV2[i].active = 0;
            }
        }
    }
}

// =============================
// 敌人增强AI（状态+攻击间隔）
// =============================

static float enemyAtkCD[MAX_ENEMY];

static void updateEnemyAI_V2(float dt) {
    for (int i = 0; i < MAX_ENEMY; i++) {
        if (!enemies[i].alive) continue;

        enemyAtkCD[i] -= dt;

        float dx = player.x - enemies[i].x;
        float dy = player.y - enemies[i].y;
        float d2 = dx*dx + dy*dy;

        if (d2 < 20.0f) {
            float len = sqrtf(d2) + 0.0001f;
            enemies[i].x += dx / len * dt * 1.5f;
            enemies[i].y += dy / len * dt * 1.5f;
        }

        if (d2 < 1.2f && enemyAtkCD[i] <= 0) {
            player.hp -= 5;
            enemyAtkCD[i] = 0.8f;
        }
    }
}

// =============================
// 轻量死亡动画（优化版）
// =============================

static void updateEnemyDeathV2() {
    for (int i = 0; i < MAX_ENEMY; i++) {
        if (enemies[i].alive) continue;

        enemies[i].hp--;

        if (enemies[i].hp < -20) {
            enemies[i].x = -100;
            enemies[i].y = -100;
        }
    }
}

// =============================
// HUD升级（武器显示）
// =============================

static void renderHUDV2() {
    char buf[128];

    snprintf(buf, sizeof(buf),
        "HP:%d Ammo:%d Weapon:%s Score:%d",
        player.hp,
        ammo,
        currentWeapon == WEAPON_PISTOL ? "PISTOL" : "SHOTGUN",
        score
    );

    for (int i = 0; i < W && buf[i]; i++)
        screen[i][H - 1] = buf[i];
}

// =============================
// 小地图优化（视野方向）
// =============================

static void renderMinimapV2() {
    int s = 11;

    for (int y = 0; y < s; y++) {
        for (int x = 0; x < s; x++) {

            int mx = (int)player.x + x - s/2;
            int my = (int)player.y + y - s/2;

            char c = '.';

            if (mx >= 0 && my >= 0 && mx < MAP_W && my < MAP_H)
                c = worldMap[mx][my] ? '#' : '.';

            screen[x][y] = c;
        }
    }

    screen[s/2][s/2] = '@';

    screen[s/2 + (int)(player.dirX*2)][s/2 + (int)(player.dirY*2)] = '>';
}

// =============================
// 主更新（最终统一入口）
// =============================

static void updateV3(float dt) {

    movePlayer(dt);
    rotatePlayer(dt);

    switchWeapon();
    fireWeapon(dt);

    updateBulletsV2(dt);
    updateEnemyAI_V2(dt);
    updateEnemyDeathV2();

    updatePickups();
    updateDoors(dt);

    handleInput(dt);
}

// =============================
// 主渲染（最终统一入口）
// =============================

static void renderV3() {

    clearBuffer();

    renderWallsV2();
    renderFloorCeil();

    renderEnemies();
    renderPickups();
    renderMinimapV2();

    renderHUDV2();
    renderDebug();

    present();
}
// =============================
// Wolf3D CLI - Part 9
// 引擎重构层（稳定架构）
// =============================

#include <time.h>

// =============================
// 游戏状态管理
// =============================

typedef enum {
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_EXIT
} GameState;

static GameState gameState = STATE_RUNNING;

// =============================
// 统一更新接口（模块化）
// =============================

static void updatePlayer(float dt) {
    movePlayer(dt);
    rotatePlayer(dt);
}

// =============================
// 统一输入处理（合并防止冲突）
// =============================

static void processInput(float dt) {

    if (key[27]) { // ESC
        gameState = STATE_EXIT;
        return;
    }

    if (key['p']) {
        gameState = STATE_PAUSED;
        key['p'] = 0;
    }

    if (key['c']) {
        gameState = STATE_RUNNING;
        key['c'] = 0;
    }

    if (gameState != STATE_RUNNING) return;

    handleBetterInput(dt);
    handleInput(dt);
}

// =============================
// 统一实体更新顺序（关键优化）
// =============================

static void updateEntities(float dt) {

    updatePlayer(dt);

    updateBulletsV2(dt);
    updateEnemyAI_V2(dt);
    updateEnemyDeathV2();

    updatePickups();
    updateDoors(dt);

    bulletHitTest();
}

// =============================
// 逻辑层总控
// =============================

static void updateWorld(float dt) {

    if (gameState != STATE_RUNNING) return;

    processInput(dt);
    updateEntities(dt);
}

// =============================
// 渲染层拆分（更清晰流水线）
// =============================

static void renderWorld() {

    renderWallsV2();
    renderFloorCeil();
}

static void renderEntitiesLayer() {

    renderEnemies();
    renderPickups();
    renderMinimapV2();
}

static void renderUI() {

    renderHUDV2();
    renderDebug();
}

// =============================
// 渲染总控（统一入口）
// =============================

static void renderFrameV4() {

    clearBuffer();

    renderWorld();
    renderEntitiesLayer();
    renderUI();

    present();
}

// =============================
// 时间稳定器（避免帧爆炸）
// =============================

static float clampDelta(float dt) {
    if (dt > 0.05f) return 0.05f;
    return dt;
}

// =============================
// 主循环升级（最终结构）
// =============================

int main() {

    srand(time(NULL));

    initEngine();
    initEntities();
    initPickups();
    generateMap();
    initBulletsV2();

    while (gameState != STATE_EXIT) {

        float dt = clampDelta(getDeltaTime());

        updateInput();
        updateWorld(dt);
        renderFrameV4();

        clearInput();
        usleep(16000);
    }

    disableRaw();
    clearScreen();

    return 0;
}
// =============================
// Wolf3D CLI - PART 10
// 最终版本（完整游戏）
// =============================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>

// =============================
// CONFIG
// =============================

#define W 80
#define H 40
#define MAP_W 24
#define MAP_H 24

#define MAX_ENEMY 32
#define MAX_BULLET 64

// =============================
// GAME STATE
// =============================

typedef enum {
    STATE_MENU,
    STATE_PLAY,
    STATE_WIN,
    STATE_LOSE
} GameState;

static GameState state = STATE_MENU;

// =============================
// INPUT
// =============================

static int key[512];

// =============================
// TERMINAL
// =============================

static struct termios orig;

static void disableRaw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

static void enableRaw() {
    tcgetattr(STDIN_FILENO, &orig);
    atexit(disableRaw);

    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void clearScreen() {
    printf("\033[2J\033[H");
}

// =============================
// TIME
// =============================

static struct timeval lastTime;

static float dt() {
    struct timeval n;
    gettimeofday(&n,NULL);

    float d =
        (n.tv_sec-lastTime.tv_sec)+
        (n.tv_usec-lastTime.tv_usec)/1000000.0f;

    lastTime=n;
    return d>0.05f?0.05f:d;
}

// =============================
// MAP
// =============================

static int map[MAP_W][MAP_H];

static void genMap() {
    for(int y=0;y<MAP_H;y++)
    for(int x=0;x<MAP_W;x++)
        map[x][y]=(x==0||y==0||x==MAP_W-1||y==MAP_H-1)?1:(rand()%100<15);
}

static int wall(int x,int y){
    if(x<0||y<0||x>=MAP_W||y>=MAP_H)return 1;
    return map[x][y];
}

// =============================
// PLAYER
// =============================

typedef struct {
    float x,y,dx,dy,px,py;
    int hp;
    int ammo;
    int score;
} Player;

static Player p;

// =============================
// ENEMY
// =============================

typedef struct {
    float x,y;
    int hp;
    int alive;
} Enemy;

static Enemy e[MAX_ENEMY];

// =============================
// BULLET
// =============================

typedef struct {
    float x,y,dx,dy;
    int active;
} Bullet;

static Bullet b[MAX_BULLET];

// =============================
// BUFFER
// =============================

static char scr[W][H];
static float zbuf[W];

// =============================
// INIT
// =============================

static void initPlayer(){
    p.x=3;p.y=3;
    p.dx=-1;p.dy=0;
    p.px=0;p.py=0.66;
    p.hp=100;
    p.ammo=20;
    p.score=0;
}

static void initEnemy(){
    for(int i=0;i<MAX_ENEMY;i++){
        e[i].x=5+i;
        e[i].y=5;
        e[i].hp=3;
        e[i].alive=1;
    }
}

static void initBullet(){
    for(int i=0;i<MAX_BULLET;i++)
        b[i].active=0;
}

// =============================
// INPUT
// =============================

static void readInput(){
    char c;
    while(read(0,&c,1)==1)
        key[(int)c]=1;
}

static void clearInput(){
    memset(key,0,sizeof(key));
}

// =============================
// MOVEMENT
// =============================

static void move(float dt){
    float m=0;
    if(key['w'])m=1;
    if(key['s'])m=-1;

    float nx=p.x+p.dx*m*5*dt;
    float ny=p.y+p.dy*m*5*dt;

    if(!wall(nx,p.y))p.x=nx;
    if(!wall(p.x,ny))p.y=ny;
}

static void rot(float dt){
    float r=0;
    if(key['a'])r=1;
    if(key['d'])r=-1;

    float a=r*2.5f*dt;

    float ox=p.dx;
    p.dx=p.dx*cosf(a)-p.dy*sinf(a);
    p.dy=ox*sinf(a)+p.dy*cosf(a);

    float op=p.px;
    p.px=p.px*cosf(a)-p.py*sinf(a);
    p.py=op*sinf(a)+p.py*cosf(a);
}

// =============================
// RAYCAST
// =============================

static float ray(float rx,float ry,int *s){
    int mx=p.x,my=p.y;

    float ddx=fabsf(1/(rx?rx:0.0001f));
    float ddy=fabsf(1/(ry?ry:0.0001f));

    int sx,sy;
    float sdx,sdy;

    if(rx<0){sx=-1;sdx=(p.x-mx)*ddx;}
    else {sx=1;sdx=(mx+1-p.x)*ddx;}

    if(ry<0){sy=-1;sdy=(p.y-my)*ddy;}
    else {sy=1;sdy=(my+1-p.y)*ddy;}

    int hit=0,side=0;

    while(!hit){
        if(sdx<sdy){sdx+=ddx;mx+=sx;side=0;}
        else {sdy+=ddy;my+=sy;side=1;}
        if(wall(mx,my))hit=1;
    }

    *s=side;
    return side? sdy-ddy : sdx-ddx;
}

// =============================
// SHOOT
// =============================

static void shoot(){
    if(!p.ammo)return;

    for(int i=0;i<MAX_BULLET;i++){
        if(!b[i].active){
            b[i].x=p.x;
            b[i].y=p.y;
            b[i].dx=p.dx;
            b[i].dy=p.dy;
            b[i].active=1;
            p.ammo--;
            break;
        }
    }
}

// =============================
// UPDATE BULLETS
// =============================

static void updateB(float dt){
    for(int i=0;i<MAX_BULLET;i++){
        if(!b[i].active)continue;

        b[i].x+=b[i].dx*10*dt;
        b[i].y+=b[i].dy*10*dt;

        if(wall(b[i].x,b[i].y))
            b[i].active=0;
    }
}

// =============================
// ENEMY AI + WIN CHECK
// =============================

static void updateE(float dt){
    int alive=0;

    for(int i=0;i<MAX_ENEMY;i++){
        if(!e[i].alive)continue;

        alive++;

        float dx=p.x-e[i].x;
        float dy=p.y-e[i].y;
        float l=sqrtf(dx*dx+dy*dy)+0.0001f;

        e[i].x+=dx/l*dt*1.2f;
        e[i].y+=dy/l*dt*1.2f;

        if(l<1.2f){
            p.hp-=5;
            if(p.hp<=0)state=STATE_LOSE;
        }
    }

    if(alive==0)
        state=STATE_WIN;
}

// =============================
// COLLISION
// =============================

static void hit(){
    for(int i=0;i<MAX_BULLET;i++){
        if(!b[i].active)continue;

        for(int j=0;j<MAX_ENEMY;j++){
            if(!e[j].alive)continue;

            float dx=b[i].x-e[j].x;
            float dy=b[i].y-e[j].y;

            if(dx*dx+dy*dy<0.3f){
                e[j].hp--;
                b[i].active=0;
                if(e[j].hp<=0){
                    e[j].alive=0;
                    p.score+=10;
                }
            }
        }
    }
}

// =============================
// RENDER
// =============================

static void clr(){
    for(int y=0;y<H;y++)
    for(int x=0;x<W;x++)
        scr[x][y]=' ';
}

static void walls(){
    for(int x=0;x<W;x++){
        float c=2*x/(float)W-1;
        float rx=p.dx+p.px*c;
        float ry=p.dy+p.py*c;

        int s;
        float d=ray(rx,ry,&s);
        zbuf[x]=d;

        int h=H/(d+0.0001f);
        int a=-h/2+H/2;
        int b=h/2+H/2;

        if(a<0)a=0;
        if(b>=H)b=H-1;

        for(int y=a;y<=b;y++)
            scr[x][y]=s?'|':'#';
    }
}

// =============================
// DRAW ENEMIES
// =============================

static void drawE(){
    for(int i=0;i<MAX_ENEMY;i++){
        if(!e[i].alive)continue;

        float dx=e[i].x-p.x;
        float dy=e[i].y-p.y;

        float inv=1.0f/(p.px*p.dy-p.dx*p.py);

        float tx=inv*(p.dy*dx-p.dx*dy);
        float ty=inv*(-p.py*dx+p.px*dy);

        if(ty<=0)continue;

        int sx=(W/2)*(1+tx/ty);
        int sz=H/ty;

        for(int y=-sz/2+H/2;y<sz/2+H/2;y++){
            if(y<0||y>=H)continue;
            if(sx<0||sx>=W)continue;
            scr[sx][y]='E';
        }
    }
}

// =============================
// UI
// =============================

static void ui(){
    char buf[128];

    snprintf(buf,sizeof(buf),
        "HP:%d Ammo:%d Score:%d",
        p.hp,p.ammo,p.score);

    for(int i=0;i<W&&buf[i];i++)
        scr[i][H-1]=buf[i];
}

// =============================
// SCREEN
// =============================

static void show(){
    printf("\033[H");
    for(int y=0;y<H;y++){
        for(int x=0;x<W;x++)
            putchar(scr[x][y]);
        putchar('\n');
    }
}

// =============================
// MAIN
// =============================

int main(){
    srand(time(NULL));

    enableRaw();
    clearScreen();

    genMap();
    initPlayer();
    initEnemy();
    initBullet();

    gettimeofday(&lastTime,NULL);

    while(state!=STATE_EXIT){

        float dt=dt();

        readInput();

        if(state==STATE_MENU){
            if(key[' ']) state=STATE_PLAY;
        }

        if(state==STATE_PLAY){
            move(dt);
            rot(dt);

            if(key[' ']){shoot();key[' ']=0;}

            updateB(dt);
            updateE(dt);
            hit();

            clr();
            walls();
            drawE();
            ui();
        }

        if(state==STATE_WIN){
            printf("\033[HYOU WIN!\nScore:%d\n",p.score);
            break;
        }

        if(state==STATE_LOSE){
            printf("\033[HYOU LOSE!\nScore:%d\n",p.score);
            break;
        }

        show();
        clearInput();
        usleep(16000);
    }

    disableRaw();
    clearScreen();
    return 0;
}
