#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "functionality.h"
#include "helper.h"

# pragma once
extern int errno;

int main()
{
#ifdef OPENFile
  FILE *pFile;
  pFile = fopen("OPENFILE","rb");
  if (pFile == NULL)
  {
    my_perror("The following error occurred");
    my_printf("Value of errno: %d\n", errno);
  }
  else {
    fclose(pFile);
  }
#endif
  
  return 0;
}
