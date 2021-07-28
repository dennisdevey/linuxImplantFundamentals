#include <stdio.h>
#include <unistd.h>


int main(int argc, char const *argv[])
{
    /* The Big Loop */
    int loopcnt = 1;
    while (1)
    {
        // #ifdef REVSH
        /* Do some task here ... */

        sleep(loopcnt); /* wait 30 seconds */
        loopcnt *= 2;
        printf("%d\n", loopcnt);
    }
    /* code */
    return 0;
}
