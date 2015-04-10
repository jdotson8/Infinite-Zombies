#ifndef GAMELIB_H
#define GAMELIB_H

#include <inttypes.h>
#include <time.h>

#define GRAPHICS_WIDTH 320
#define GRAPHICS_HEIGHT 240

typedef enum {TINY=1, SMALL=2, MEDIUM=3, LARGE=4, HOOPLA=5} fontsize_t;

void outputFrame(int fd, uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH]);
void waitTimer();
void initTimer(float fps);
void stopTimer();
void enableInput();
void disableInput();
int getKeyPress();

#endif
