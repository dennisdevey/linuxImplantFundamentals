#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <string.h>


int main( int argc, char *argv[] )
{
  char * giantList = malloc(1024);
  struct utsname uts;
  uname(&uts);
#ifdef DEBUG
  printf("%s:%s\n",uts.sysname, uts.machine);
  printf("%s\n",uts.release);
  printf("%s\n",uts.version);
#endif

  strcat(giantList,uts.sysname);
  strcat(giantList,"//");

  strcat(giantList,uts.machine);
  strcat(giantList,"//");

  strcat(giantList,uts.release);
  strcat(giantList,"//");

  strcat(giantList,uts.version);
  strcat(giantList,"//");

  FILE *fp;
  char path[1035];
  fp = popen("id", "r");

 
  fgets(path, sizeof(path), fp);
#ifdef DEBUG
  printf("%s", path);
#endif
  strcat(giantList, path);
  strcat(giantList,"//");

  
  
  fp = popen("ldd --version", "r");

  fgets(path, sizeof(path), fp); 
#ifdef DEBUG
  printf("%s", path);
#endif
  strcat(giantList, path);
  strcat(giantList,"//");

  fp = popen("localectl status", "r");

  fgets(path, sizeof(path), fp);
#ifdef DEBUG
  printf("%s", path);
#endif
  strcat(giantList, path);
  strcat(giantList,"//");

  printf("%s", giantList); 

  /* close */
  pclose(fp);

  return 0;
}
