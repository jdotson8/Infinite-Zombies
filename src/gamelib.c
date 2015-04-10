#include <linux/fb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <curses.h>
#include "gamelib.h"
#include "text.h"

// Writes an entire frame out to the frame buffer device
void outputFrame(int fd, uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH])
{
    lseek(fd, 0, SEEK_SET);
    if (write(fd, frame, GRAPHICS_WIDTH * GRAPHICS_HEIGHT * sizeof(uint16_t)) == -1) {
        fprintf(stderr, "Error writing to framebuffer device.\n\r");
        refresh();
    }
}

// Global flag for the timer interrupt to set
volatile bool timerExpired = false;

// Timer interrupt handler
void onTimerExpire(int sig)
{
    timerExpired = true;
}
// Waits for the next timer interrupt
void waitTimer()
{
    while(!timerExpired);
    timerExpired = false;
}

timer_t timer;

// Set up a frame rate timer
void initTimer(float fps)
{
    // Calculate the desired period
    struct timespec framePeriod;
    framePeriod.tv_sec = 0;
    framePeriod.tv_nsec = 1e9 / fps;
    struct itimerspec timerSpec;
    timerSpec.it_interval = framePeriod;
    timerSpec.it_value = framePeriod;
    // Trigger SIGALRM when the timer expires
    struct sigevent eventSpec;
    eventSpec.sigev_notify = SIGEV_SIGNAL;
    eventSpec.sigev_signo = SIGALRM;
    // Create the timer
    timer_create(CLOCK_REALTIME, &eventSpec, &timer);
    timer_settime(timer, 0, &timerSpec, NULL);
    // Set up a signal handler for the expire event
    struct sigaction sa;
    sa.sa_handler = &onTimerExpire;
    sigaction(SIGALRM, &sa, NULL);
}

void stopTimer()
{
    timer_delete(timer);
}

void enableInput() {
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    noecho();
    nl();
}

void disableInput() {
    endwin();
}

int getKeyPress() {
    int ch = getch();
    // allow only a 1 char buffer
    // this way we don't fill up stdin (key presses continue after you let go of the button)
    // but still have input every frame when we hold down a button
    int next = getch();
    flushinp();
    if (next != -1) {
        ungetch(next);
    }
    return ch;
}
