/* This makes a big ole string of // separated fields that could be useful for profiling. 
Easier than that, if you want to use this as a validator, you can just check to see that values 
are equal to what you want them to be and return true / false */

// an example of this is located with UTS_SYSNAME

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <string.h>
#define UTS_SYSNAME "Linux"

int main( int argc, char *argv[] )
{
  
  struct utsname uts;
  uname(&uts);
#ifdef UTS_SYSNAME
  if (strcmp(uts.sysname, UTS_SYSNAME)==0){
    return 0;
  else {
    return -1;
  }
#endif
#ifdef DEBUG
  printf("%s:%s\n",uts.sysname, uts.machine);
  printf("%s\n",uts.release);
  printf("%s\n",uts.version);
#endif
#ifdef STRING
  char * giantList = malloc(1024);
  strcat(giantList,uts.sysname);
  strcat(giantList,"//");

  strcat(giantList,uts.machine);
  strcat(giantList,"//");

  strcat(giantList,uts.release);
  strcat(giantList,"//");

  strcat(giantList,uts.version);
  strcat(giantList,"//");
#endif

  FILE *fp;
  char path[1035];
  fp = popen("id", "r");

 
  fgets(path, sizeof(path), fp);
#ifdef DEBUG
  printf("%s", path);
#endif
#ifdef STRING
  strcat(giantList, path);
  strcat(giantList,"//");
#endif
  
  
  fp = popen("ldd --version", "r");

  fgets(path, sizeof(path), fp); 
#ifdef DEBUG
  printf("%s", path);
#endif
#ifdef STRING
  strcat(giantList, path);
  strcat(giantList,"//");
#endif
  fp = popen("localectl status", "r");

  fgets(path, sizeof(path), fp);
#ifdef DEBUG
  printf("%s", path);
#endif
#ifdef STRING
  strcat(giantList, path);
  strcat(giantList,"//");
#endif
#ifdef DEBUG
  printf("%s", giantList); 
#endif
  /* close */
  pclose(fp);

  return 0;
}
