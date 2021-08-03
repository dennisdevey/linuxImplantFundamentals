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
#ifdef ARCH
	printf("Architecture type is %s\n", ARCH);
#endif
#ifdef OS
	printf("Operating System is %s\n", OS);
#endif
#ifdef LIBV
	printf("Libc Version is %s\n", LIBV);
#endif
#ifdef KERV
        printf("Kernel  Version is %s\n", KERV);
#endif
#ifdef FUNC
        printf("Functionality is %s\n", FUNC);
#endif
#ifdef EXEC
        printf("Execution Guardrails are  %s\n", EXEC);
#endif
#ifdef PERM
        printf("Persistence Mechanism is %s\n", PERM);
#endif
#ifdef SYSN
	printf("System name is %s\n", SYSN);
#endif
	return 0;
}

