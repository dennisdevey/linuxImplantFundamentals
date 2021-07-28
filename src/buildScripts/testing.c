#include <stdio.h>

// #define IPADDR "192.168.1.1"

int main(int argc, char const *argv[])
{
    #ifdef HELLO
    printf("Hello world\n");
    #endif

    #ifdef FOOBAR
    printf("foobar\n");
    #endif

    #ifdef DEBUG
    printf("DEBUGGING ON\n");
    #endif

    #ifdef IPADDR
    printf("IP is %s\n", IPADDR);
    #endif

    return 0;
}
