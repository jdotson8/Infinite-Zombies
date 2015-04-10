#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <math.h>
#include "gamelib.h"
#include "text.h"
#include "level.c"
#include "wall.c"
#include "floor.c"
#include "playerwalk.c"
#include "playershoot.c"
#include "zombiewalk.c"
#include "zombieattack.c"
#define TRANSPARENT 0x07E0
#define TILE_X 32
#define TILE_Y 16
#define TILE_WIDTH 64
#define TILE_HEIGHT 72
#define OUT_OF_BOUNDS 4
#define GRAPHICS_FRAME uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH]
#define GRAPHICS_ZBUFFER int zbuff[GRAPHICS_HEIGHT][GRAPHICS_WIDTH]

static const char *device = "/dev/fb1";
                            
struct pair {
    int x;
    int y;
};

struct player {
    float x;
    float y;
    float dx;
    float dy;
    float speed;
    float anim;
    int canMove;
    int moving;
    int shooting;
    int facing;
    int health;
    int kills;
    struct pair next;
    struct world *world;
};

struct zombie {
    float x;
    float y;
    float dx;
    float dy;
    float speed;
    float playerDistance;
    float anim;
    int canMove;
    int moving;
    int facing;
    int state;
    int attackTimer;
    struct pair next;
    struct world *world;
};

struct world {
    int **occupied;
    struct player player;
    struct zombie *zombies;
    int zombieCount;
    int width;
    int height;
};
    

/*void drawImage(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, int x, int y, int width, int height, int depth, float shade, const uint8_t *pixels);
void drawSprite(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, int x, int y, int width, int height, int row, int col, int depth, float shade, int sheetWidth, const uint8_t *spriteSheet);
void drawPlayer(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, struct player player);
void drawZombie(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, struct zombie zombie);
void drawChar(GRAPHICS_FRAME, int x, int y, char c, uint16_t color, fontsize_t fontsize);
void drawString(GRAPHICS_FRAME, int x, int y, char *string, uint16_t color, fontsize_t fontsize);
uint16_t colorMul(uint16_t color, float scalar);
float distanceToPlayer(struct player player, float x, float y);
struct zombie* getZombieAt(struct world *world, struct pair at);
void initPlayer(struct world *world, struct player *player, int x, int y);
void initZombie(struct world *world, struct zombie *zombie, int x, int y);
void updatePlayer(struct player *player, int pressed);
void updateZombie(struct zombie *zombie);
void killZombieAt(struct world *world, struct pair at);
struct pair lineOfSight(struct world *world, int xfrom, int yfrom, int dir);
int spaceOccupied(struct world *world, int x, int y);
int sign(float num);
void attack(struct player *player, int damage);
int toScreenX(float x, float y);
int toScreenY(float x, float y);
int max(int num1, int num2);
int min(int nim1, int num2);*/

void initPlayer(struct world *world, struct player *player, int x, int y) {
    player->x = x;
    player->y = y;
    player->next.x = x;
    player->next.y = y;
    player->speed = 8;
    player->anim = 0.0;
    player->canMove = 1;
    player->moving = 0;
    player->shooting = 0;
    player->facing = 2;
    player->health = 100;
    player->kills = 0;
    player->world = world;
}

float distanceToPlayer(struct player player, float x, float y) {
    return sqrt(pow(player.x - x, 2) + pow(player.y - y, 2));
}

void initZombie(struct world *world, struct zombie *zombie, int x, int y) {
    zombie->x = x;
    zombie->y = y;
    zombie->next.x = x;
    zombie->next.y = y;
    zombie->speed = 16;
    zombie->playerDistance = distanceToPlayer(world->player, x, y);
    zombie->anim = 0.0;
    zombie->canMove = 1;
    zombie->moving = 0;
    zombie->facing = 2;
    zombie->state = 0;
    zombie->attackTimer = 20;
    zombie->world = world;
}

struct world* createWorld(int width, int height, const int level[height][width]) {
    struct world *world = malloc(sizeof(struct world));
    world->zombies = malloc(sizeof(struct zombie) * 25);
    world->zombieCount = 0;
    world->occupied = malloc(sizeof(int*) * height);
    int x, y;
    for (y = 0; y < height; y++) {
        world->occupied[y] = malloc(sizeof(int*) * width);
        for (x = 0; x < width; x++) {
            world->occupied[y][x] = level[y][x];
            if (level[y][x] == 3) {
                initPlayer(world, &world->player, x, y);
            }
        }
    }
    world->width = width;
    world->height = height;
    return world;
}

int max(int num1, int num2) {
    return num1 >= num2 ? num1 : num2;
}

int min(int num1, int num2) {
    return num1 <= num2 ? num1 : num2;
}

struct zombie* getZombieAt(struct world *world, struct pair at) {
    int i;
    for (i = 0; i < world->zombieCount; i++) {
        if (world->zombies[i].next.x == at.x && world->zombies[i].next.y == at.y) {
            return &world->zombies[i];
        }
    }
    return 0;
}

void killZombieAt(struct world *world, struct pair at) {
    int i;
    for (i = 0; i < world->zombieCount; i++) {
        if (world->zombies[i].next.x == at.x && world->zombies[i].next.y == at.y) {
            world->occupied[at.y][at.x] = 0;
            world->zombieCount--;
            world->zombies[i] = world->zombies[world->zombieCount];
            return;
        }
    }
}

struct pair lineOfSight(struct world *world, int xfrom, int yfrom, int dir) {
    struct pair hit;
    int i;
    switch (dir) {
        case 0:
            for (i = yfrom - 1; i >= 0; i--) {
                if (world->occupied[i][xfrom]) {
                    hit.x = xfrom;
                    hit.y = i;
                    return hit;
                }
            }
            break;
        case 1:
            for (i = xfrom - 1; i >= 0; i--) {
                if (world->occupied[yfrom][i]) {
                    hit.x = i;
                    hit.y = yfrom;
                    return hit;
                }
            }
            break;
        case 2:
            for (i = yfrom + 1; i < world->height; i++) {
                if (world->occupied[i][xfrom]) {
                    hit.x = xfrom;
                    hit.y = i;
                    return hit;
                }
            }
            break;
        case 3:
            for (i = xfrom + 1; i < world->width; i++) {
                if (world->occupied[yfrom][i]) {
                    hit.x = i;
                    hit.y = yfrom;
                    return hit;
                }
            }
            break;
    }
    hit.x = -1;
    return hit;
}

int spaceOccupied(struct world *world, int x, int y) {
    if (x >= 0 && x < world->width && y >= 0 && y < world->height) {
        return world->occupied[y][x];
    } else {
        return OUT_OF_BOUNDS;
    }
}

int sign(float num) {
    if (num == 0) {
        return 0;
    }
    return num > 0 ? 1 : -1;
}

void attack(struct player *player, int damage) {
    if (player->health > 0) {
        player->health -= min(damage, player->health);
    }
}

void updatePlayer(struct player *player, int pressed) {
    if (player->canMove) {
        struct pair next;
        next.x = roundf(player->x);
        next.y = roundf(player->y);
        switch (pressed) {
            case 'w':
                player->facing = 0;
                next.y -= 1;
                break;
            case 'a':
                player->facing = 1;
                next.x -= 1;
                break;
            case 's':
                player->facing = 2;
                next.y += 1;
                break;
            case 'd':
                player->facing = 3;
                next.x += 1;
                break;
            case ' ':
                player->shooting = 1;
                player->anim = 0;
                player->canMove = 0;
                struct pair hit = lineOfSight(player->world, player->x, player->y, player->facing);
                if (spaceOccupied(player->world, hit.x, hit.y) == 2) {
                    killZombieAt(player->world, hit);
                    player->kills++;
                }
                break;
        }
        if (next.x != player->x || next.y != player->y) {
            if (!spaceOccupied(player->world, next.x, next.y)) {
                player->world->occupied[(int)player->y][(int)player->x] = 0;
                player->world->occupied[next.y][next.x] = 3;
                player->next = next;
                player->moving = 1;
                player->canMove = 0;
                player->dx = (player->next.x - (int)player->x) / player->speed;
                player->dy = (player->next.y - (int)player->y) / player->speed;
            }
        }
        if (player->canMove || player->shooting) {
            player->moving = 0;
            if (player->canMove) {
                player->anim = 0;
            }
        }
    }
    if (player->moving) {
        player->x += player->dx;
        player->y += player->dy;
        player->anim += 4 / player->speed;
        if ((player->x - player->next.x) * sign(player->dx) >= 0 && (player->y - player->next.y) * sign(player->dy) >= 0) {
            player->x = player->next.x;
            player->y = player->next.y;
            player->canMove = 1;
        }
    }
    if (player->shooting) {
        player->anim += 1.0;
        if (player->anim >= 4) {
            player->shooting = 0;
            player->canMove = 1;
            player->anim = 0;
        }
    }
}

void updateZombie(struct zombie *zombie) {
    if (zombie->state == 0) {
        if (zombie->playerDistance <= 9) {
            if (zombie->playerDistance <= 3) {
                zombie->state++;
            } else {
                struct pair seen = lineOfSight(zombie->world, zombie->x, zombie->y, zombie->facing);
                if (spaceOccupied(zombie->world, seen.x, seen.y) == 3) {
                    zombie->state++;
                } else if (spaceOccupied(zombie->world, seen.x, seen.y) == 2) {
                    if (getZombieAt(zombie->world, seen)->state == 1) {
                        zombie->state++;
                    }
                }
            }
        }
    }
    if (zombie->canMove) {
        struct pair next;
        next.x = roundf(zombie->x);
        next.y = roundf(zombie->y);
        int nextSet = 0;
        if (zombie->state == 0) {
            int wait = rand() % 20;
            if (!wait) {
                switch(rand() % 4) {
                    case 0:
                        zombie->facing = 0;
                        next.y -= 1;
                        break;
                    case 1:
                        zombie->facing = 1;
                        next.x -= 1;
                        break;
                    case 2:
                        zombie->facing = 2;
                        next.y += 1;
                        break;
                    case 3:
                        zombie->facing = 3;
                        next.x += 1;
                        break;
                }
                if (!spaceOccupied(zombie->world, next.x, next.y)) {
                    nextSet = 1;
                }
            }
        } else if (zombie->state == 1) {
            int xdiff = roundf(zombie->world->player.x) - zombie->x;
            int ydiff = roundf(zombie->world->player.y) - zombie->y;
            if (abs(xdiff) + abs(ydiff) == 1) {
                zombie->state++;
                zombie->anim = 0;
                zombie->attackTimer = 1;
                zombie->canMove = 0;
            } else {
                if (zombie->playerDistance > 9) {
                    zombie->state--;
                } else if (abs(xdiff) >= abs(ydiff)) {
                    if (!spaceOccupied(zombie->world, next.x + sign(xdiff), next.y)) {
                        zombie->facing = 2 + sign(xdiff);
                        next.x += sign(xdiff);
                        nextSet = 1;
                    } else if (!spaceOccupied(zombie->world, next.x, next.y + sign(ydiff)) && sign(ydiff)) {
                        zombie->facing = 1 + sign(ydiff);
                        next.y += sign(ydiff);
                        nextSet = 1;
                    }
                } else {
                    if (!spaceOccupied(zombie->world, next.x, next.y + sign(ydiff))) {
                        zombie->facing = 1 + sign(ydiff);
                        next.y += sign(ydiff);
                        nextSet = 1;
                    } else if (!spaceOccupied(zombie->world, next.x + sign(xdiff), next.y) && sign(xdiff)) {
                        zombie->facing = 2 + sign(xdiff);
                        next.x += sign(xdiff);
                        nextSet = 1;
                    }
                }
            }
        }
        if (nextSet) {
            zombie->world->occupied[(int)zombie->y][(int)zombie->x] = 0;
            zombie->world->occupied[next.y][next.x] = 2;
            zombie->next = next;
            zombie->moving = 1;
            zombie->canMove = 0;
            zombie->dx = (zombie->next.x - zombie->x) / zombie->speed;
            zombie->dy = (zombie->next.y - zombie->y) / zombie->speed;
        }
        if (zombie->canMove) {
            zombie->moving = 0;
            if (zombie->state != 2) {
                zombie->anim = 0;
            }
        }
    }
    if (zombie->moving) { 
        zombie->x += zombie->dx;
        zombie->y += zombie->dy;
        zombie->anim += 4 / zombie->speed;
        if ((zombie->x - zombie->next.x) * sign(zombie->dx) >= 0 && (zombie->y - zombie->next.y) * sign(zombie->dy) >= 0) {
            zombie->x = zombie->next.x;
            zombie->y = zombie->next.y;
            zombie->canMove = 1;
        }
    }
    if (zombie->state == 2) {
        zombie->attackTimer--;
        zombie->anim += 0.5;
        if (zombie->anim >= 4) {
            zombie->anim = 3;
        }
        if(!zombie->attackTimer) {
            attack(&zombie->world->player, 5);
            zombie->attackTimer = 40;
            zombie->anim = 0;
        }
        int xdiff = roundf(zombie->world->player.x) - zombie->x;
        int ydiff = roundf(zombie->world->player.y) - zombie->y;
        zombie->facing = abs(xdiff) >= abs(ydiff) ? 2 + sign(xdiff) : 1 + sign(ydiff);
        if (abs(xdiff) + abs(ydiff) != 1) {
            zombie->state--;
        }
    }
    zombie->playerDistance = distanceToPlayer(zombie->world->player, zombie->x, zombie->y);
}

void updateWorld(struct world *world, int pressed) {
    updatePlayer(&world->player, pressed);
    int i;
    for(i = 0; i < world->zombieCount; i++) {
        updateZombie(&world->zombies[i]);
    }
    int wait = rand() % 20;
    if (!wait && world->zombieCount < 25) {
        int x = rand() % world->width;
        int y = rand() % world->height;
        if(!world->occupied[y][x]) {
            world->occupied[y][x] = 2;
            initZombie(world, &world->zombies[world->zombieCount], x, y);
            world->zombieCount++;
        }
    }
}

void initFrame(GRAPHICS_FRAME, uint16_t color)
{
    int i, j;
    for (i = 0; i < GRAPHICS_HEIGHT; i++) {
        for (j = 0; j < GRAPHICS_WIDTH; j++){
            frame[i][j] = color;
        }
    }
}

void clearDepthBuffer(GRAPHICS_ZBUFFER) {
    int i, j;
    for (i = 0; i < GRAPHICS_HEIGHT; i++) {
        for (j = 0; j < GRAPHICS_WIDTH; j++){
            zbuff[i][j] = 0;
        }
    }
}

int toScreenX(float x, float y)  {
    return TILE_X * x - TILE_X * y + TILE_X - TILE_WIDTH;
}

int toScreenY(float x, float y)  {
    return TILE_Y * x + TILE_Y * y + TILE_Y - TILE_HEIGHT;
}

uint16_t colorMul(uint16_t color, float scalar) {
    int r = color >> 11;
    int g = (color >> 5) & 0x003F;
    int b = color & 0x001F;
    r *= scalar;
    g *= scalar;
    b *= scalar;
    return (r << 11) | (g << 5) | b;
}

void drawRect(GRAPHICS_FRAME, int x, int y, int width, int height, uint16_t color)
{
    if (x + width >= 0  && x < GRAPHICS_WIDTH && y + height >= 0 && y < GRAPHICS_HEIGHT) {
        int xmin = max(0, x);
        int xmax = min(x + width, GRAPHICS_WIDTH);
        int ymin = max(0, y);
        int ymax = min(y + height, GRAPHICS_HEIGHT);
        int i, j;
        for (j = ymin; j < ymax; j++) {
            for (i = xmin; i < xmax; i++) {
                frame[j][i] = color;
            }
        }
    }
}

void drawImage(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, int x, int y, int width, int height, int depth, float shade, const uint8_t *pixels) {
    if (x + width >= 0  && x < GRAPHICS_WIDTH && y + height >= 0 && y < GRAPHICS_HEIGHT) {
        int xmin = max(0, -x);
        int xmax = min(width, GRAPHICS_WIDTH - x);
        int ymin = max(0, -y);
        int ymax = min(height, GRAPHICS_HEIGHT - y);
        uint16_t *image = (uint16_t*) pixels;
        int i,j;
        for (j = ymin; j < ymax; j++) {
            for (i = xmin; i < xmax; i++) {
                if (image[j * width + i] != TRANSPARENT && depth >= zbuff[j + y][i + x] ) {
                    zbuff[j + y][i + x] = depth;
                    frame[j + y][i + x] = colorMul(image[j * width + i], shade);
                }
            }
        }
    }
}

void drawSprite(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, int x, int y, int width, int height, int row, int col, int depth, float shade, int sheetWidth, const uint8_t *spriteSheet) {
    int offsetx = col * width;
    int offsety = row * height;
    if (x + width >= 0  && x < GRAPHICS_WIDTH && y + height >= 0 && y < GRAPHICS_HEIGHT) {
        int xmin = offsetx + max(0, -x);
        int xmax = offsetx + min(width, GRAPHICS_WIDTH - x);
        int ymin = offsety + max(0, -y);
        int ymax = offsety + min(height, GRAPHICS_HEIGHT - y);
        uint16_t *image = (uint16_t*) spriteSheet;
        int i,j;
        for (j = ymin; j < ymax; j++) {
            for (i = xmin; i < xmax; i++) {
                if (image[j * sheetWidth + i] != TRANSPARENT && depth >= zbuff[j - offsety + y][i - offsetx + x] ) {
                    zbuff[j - offsety + y][i - offsetx + x] = depth;
                    frame[j - offsety + y][i - offsetx + x] = colorMul(image[j * sheetWidth + i], shade);
                }
            }
        }
    }
}

void drawChar(GRAPHICS_FRAME, int x, int y, char c, uint16_t color, fontsize_t fontsize)
{
    int width = fontsize * char_width;
    int height = fontsize * char_height;
    if (x + width >= 0  && x < GRAPHICS_WIDTH && y + height >= 0 && y < GRAPHICS_HEIGHT) {
        int xmin = max(0, x);
        int xmax = min(x + width, GRAPHICS_WIDTH);
        int ymin = max(0, y);
        int ymax = min(y + height, GRAPHICS_HEIGHT);
        int i, j;
        for(j = ymin; j < ymax; j++) {
            for(i = xmin; i < xmax; i++) {
                if (charHasPixelSet(c, (j - ymin)/ fontsize, (i - xmin)/ fontsize)) {
                    frame[j][i] = color;
                }
            }
        }
    }
}

void drawString(GRAPHICS_FRAME, int x, int y, char *string, uint16_t color, fontsize_t fontsize)
{   
    int i = 0;
    while(string[i] != 0) {
        drawChar(frame, x + i * (char_width + 1) * fontsize, y, string[i], color, fontsize);
        i++;
    }
}

void drawPlayer(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, struct player player) {
    int screenx = GRAPHICS_WIDTH / 2 + TILE_X - TILE_WIDTH;
    int screeny = GRAPHICS_HEIGHT / 2 + TILE_Y - TILE_HEIGHT;
    if (player.moving) {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, player.facing, ((int)player.anim) % 4, player.x + player.y + 1, 1, playerWalk.width, playerWalk.pixel_data);
    } else if (player.shooting) {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, player.facing, ((int)player.anim) % 4, player.x + player.y + 1, 1, playerShoot.width, playerShoot.pixel_data);
    } else {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, player.facing, 0, player.x + player.y + 1, 1, playerWalk.width, playerWalk.pixel_data);
    }
}

void drawZombie(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, struct zombie zombie) {
    int screenx = toScreenX(zombie.x - zombie.world->player.x, zombie.y - zombie.world->player.y) + GRAPHICS_WIDTH / 2;
    int screeny = toScreenY(zombie.x - zombie.world->player.x, zombie.y - zombie.world->player.y) + GRAPHICS_HEIGHT / 2;
    float shade = 1.0 / zombie.playerDistance + zombie.world->player.shooting * (4 - zombie.world->player.anim) * 0.025 + 0.25;
    shade = shade > 1 ? 1 : shade;
    if (zombie.moving == 1) {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, zombie.facing, ((int)zombie.anim) % 4, zombie.x + zombie.y + 1, shade, zombieWalk.width, zombieWalk.pixel_data);
    } else if (zombie.state == 2) {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, zombie.facing, ((int)zombie.anim) % 4, zombie.x + zombie.y + 1, shade, zombieAttack.width, zombieAttack.pixel_data);
    } else {
        drawSprite(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, zombie.facing, 0, zombie.x + zombie.y + 1, shade, zombieWalk.width, zombieWalk.pixel_data);
    }
}

void drawWorld(GRAPHICS_FRAME, GRAPHICS_ZBUFFER, struct world *world) {
    int x, y;
    for(y = world->height - 1; y >= 0; y--) {
        for(x = world->width - 1; x >= 0; x--) {
            int screenx, screeny;
            float shade;
            screenx = toScreenX(x - world->player.x, y - world->player.y) + GRAPHICS_WIDTH / 2;
            screeny = toScreenY(x - world->player.x, y - world->player.y) + GRAPHICS_HEIGHT / 2;
            if (screenx + TILE_WIDTH >= 0  && screenx < GRAPHICS_WIDTH && screeny + TILE_HEIGHT >= 0 && y < GRAPHICS_HEIGHT) {
                shade = 1.0 / distanceToPlayer(world->player, x, y) + world->player.shooting * (4 - world->player.anim) * 0.025 + 0.25;
                shade = shade > 1 ? 1 : shade;
                switch (world->occupied[y][x]) {
                    case 1:
                        drawImage(frame, zbuff, screenx, screeny,  TILE_WIDTH, TILE_HEIGHT, x + y + 1, shade, wallTex.pixel_data);
                        break;
                    default:
                        drawImage(frame, zbuff, screenx, screeny, TILE_WIDTH, TILE_HEIGHT, x + y, shade, floorTex.pixel_data);
                        break;
                }
            }
        }
    }
    drawPlayer(frame, zbuff, world->player);
    int i;
    for (i = 0; i < world->zombieCount; i++) {
        drawZombie(frame, zbuff, world->zombies[i]);
    }
    drawRect(frame, 0, 0, GRAPHICS_WIDTH, 26, 0x0000);
    drawRect(frame, 8, 8, 66, 10, 0x0000);
    drawRect(frame, 9, 9, 64 * (float) world->player.health / 100, 8, 0x07E0);
    
    char kills[10];
    snprintf(kills, 10, "Kills: %d", world->player.kills);
    drawString(frame, 82, 8, kills, 0xFFFF, TINY);
    drawString(frame, GRAPHICS_WIDTH - 80, 8, "[1] Pause", 0xFFFF, TINY);
}

void freeWorld(struct world *world) {
    int i;
    for (i = 0; i < world->height; i++) {
            free(world->occupied[i]);
    }
    free(world->occupied);
    free(world->zombies);
    free(world);
}
    

int main(int argc, char **argv)
{
    // Initialize graphics
    int fd = open(device, O_RDWR);
    GRAPHICS_FRAME;
    GRAPHICS_ZBUFFER;
    struct world *world = createWorld(25, 25, LEVEL);
    int state = 0;
    int quit = 0;
    int pressed;
    enableInput();
    initTimer(20.0);

    // Main loop
    while(!quit) {
        pressed = getKeyPress();
        switch(state) {
            case 0:
                initFrame(frame, 0x0000);
                drawString(frame, 16, 16, "ENDLESS", 0xFFFF, HOOPLA);
                drawString(frame, 16, 81, "ZOMBIES", 0xFFFF, HOOPLA);
                drawString(frame, GRAPHICS_WIDTH /2, 146, "By Josh Dotson", 0xFFFF, TINY);
                drawString(frame, 16, 162, "[1] Start", 0xFFFF, SMALL);
                drawString(frame, 16, 188, "[2] Instructions", 0xFFFF, SMALL);
                drawString(frame, 16, 214, "[3] Quit", 0xFFFF, SMALL);
                
                switch(pressed) {
                    case '1':
                        state = 2;
                        break;
                    case '2':
                        state = 1;
                        break;
                    case '3':
                        quit = 1;
                        break;
                }
                break;
            case 1:
                initFrame(frame, 0x0000);
                drawString(frame, 16, 16, "Endless zombies is a simple arcade", 0xFFFF, TINY);
                drawString(frame, 16, 29, "zombie shooter. Move with WASD and", 0xFFFF, TINY);
                drawString(frame, 16, 42, "shoot with SPACE.", 0xFFFF, TINY);
                drawString(frame, 16, 188, "[1] Back", 0xFFFF, SMALL);
                drawString(frame, 16, 214, "[2] Quit", 0xFFFF, SMALL);
                
                switch(pressed) {
                    case '1':
                        state = 0;
                        break;
                    case '2':
                        quit = 1;
                        initFrame(frame, 0x0000);
                        break;
                }
                break;
            case 2:
                if (pressed == '1') {
                    state = 3;
                } else {
                    updateWorld(world, pressed);
                    initFrame(frame, 0x0000);
                    clearDepthBuffer(zbuff);
                    drawWorld(frame, zbuff, world);
                    if (world->player.health == 0) {
                        freeWorld(world);
                        state = 4;
                    }
                }
                break;
            case 3:
                drawString(frame, 16, 16, "PAUSED", 0xFFFF, HOOPLA);
                drawString(frame, 16, 188, "[1] Resume", 0xFFFF, SMALL);
                drawString(frame, 16, 214, "[2] Quit", 0xFFFF, SMALL);
                
                switch(pressed) {
                    case '1':
                        state = 2;
                        break;
                    case '2':
                        freeWorld(world);
                        quit = 1;
                        initFrame(frame, 0x0000);
                        break;
                }
                break;
            case 4:
                drawString(frame, 16, 16, "GAME", 0xFFFF, HOOPLA);
                drawString(frame, 16, 81, "OVER", 0xFFFF, HOOPLA);
                drawString(frame, 16, 188, "[1] Restart", 0xFFFF, SMALL);
                drawString(frame, 16, 214, "[2] Quit", 0xFFFF, SMALL);
                
                switch(pressed) {
                    case '1':
                        world = createWorld(25, 25, LEVEL);
                        state = 2;
                        break;
                    case '2':
                        quit = 1;
                        initFrame(frame, 0x0000);
                        break;
                }
                break;
        }
        waitTimer();
        outputFrame(fd, frame);
    }
    stopTimer();
    disableInput();
    close(fd);
    return 0;
}