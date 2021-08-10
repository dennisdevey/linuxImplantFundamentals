#pragma once

#include "helper.h"

void my_printf(char *msg)
{
#ifdef DEBUG
    printf("%s", msg);
#endif
}

void my_perror(char *msg)
{
#ifdef DEBUG
    perror(msg);
#endif
}
