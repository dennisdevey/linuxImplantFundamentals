#include <stdio.h>

int main()
{
#ifdef DEBUG
	printf("Debugging in Process");
	printf("\n");
#endif
#ifdef IPNUM
	printf("IP Address is %s\n", IPNUM);
#endif
	return 0;
}

