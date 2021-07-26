#include <stdio.h>
#include <errno.h>
#include "functionality.h"
#include "helper.h"

extern int errno;

int main ()
{
  #ifdef openFile
  FILE * pFile;
  pFile = fopen ("unexist.ent","rb");
  if (pFile == NULL)
  {
    my_perror ("The following error occurred");
    my_printf( "Value of errno: %d\n", errno );
  }
  else {
    fclose (pFile);
  }
  #endif
  
  return 0;
  
}
