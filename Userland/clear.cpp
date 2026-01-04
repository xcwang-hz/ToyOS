#include <stdio.h>

int main(int, char**)
{
    printf("\033[3J\033[H\033[2J");
    printf("clear\n");
    return 0;
}