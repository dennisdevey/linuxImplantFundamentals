#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


int main(int argc, char *argv[])
{
    while (1)
    {
        printf("Enter your command: ");
        char cmd[256];
        scanf("%s", cmd);

        if (strcmp(cmd, "SHELL") == 0)
        {
            /* code */
            int proc = fork();
            if (proc >= 1)
            {               //parent
                wait(NULL); //wait until child terminates
            }
            else if (proc == 0)
            { //child process
                execl("/bin/sh", "sh", NULL);
            }
        }
        else if (strcmp(cmd, "UNINSTALL") == 0)
        {
            remove(argv[0]);
        }
        else if (strcmp(cmd, "SLEEP") == 0)
        {
            int sleeptime = 0;
            // printf("How long? ");
            scanf("%d", &sleeptime);
            sleep(sleeptime);
        }
        else if (strcmp(cmd, "PROFILER") == 0)
        {
            // TODO: Implement the profiler
            printf("Implement the profiler lol\n");
        }
        else if (strcmp(cmd, "EXIT") == 0)
        {
            kill(getpid(), SIGKILL);
        }
        else {
            printf("Unrecognized command.\n");
        }
    }

    return 0;
}
