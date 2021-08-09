#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "client/helper.h"

#define MUTEX "/tmp/.keepalive_root"

int mutexchk()
{
    if (access(MUTEX, F_OK) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void rmmutex()
{
    remove(MUTEX);
}

void mkmutex()
{
    mode_t mode = S_IWUSR | S_IWGRP | S_IWOTH;
    if (creat(MUTEX, mode) < 0)
    {
        my_perror("File creation error.");
    }
}

// FOR TESTING ONLY
// int main(int argc, char const *argv[])
// {
//     if (mutexchk())
//     {
//         printf("Mutex present\n");
//         rmmutex();
//     }
//     else
//     {
//         printf("Mutex absent\n");
//         mkmutex();
//     }

//     return 0;
// }
