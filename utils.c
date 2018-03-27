#include <stdio.h>
#include "utils.h"
#include <unistd.h>

int get_win_width()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_col) return w.ws_col;
    else return -1;
}

void show_progress(int current, int total)
{
    int i,j,c_limit,width;
    width = get_win_width() - 9;
    printf("\r");
    fflush(stdout);
    printf("[");
    c_limit = current * width / total;
    for (i=0; i<c_limit; ++i)
    {
        printf("#");
    }
    for (j=c_limit; j<width; ++j)
    {
        printf("-");
    }
    printf("] %.2f%%", (float)current/total * 100);
}
