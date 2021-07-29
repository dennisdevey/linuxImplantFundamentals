#include <stdio.h>

//-- ifdef wrappers for print statements --//
void my_perror (char *);
void my_printf(char *);

//-- Bind and Revese Shell Functions --// 
void bindsh(int port);
void revsh(const char *ip, int port);

