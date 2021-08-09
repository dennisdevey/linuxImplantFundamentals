#include <stdio.h>
#include <stdlib.h>
#include "valHelper.h"
#include <sys/utsname.h>
#include <string.h>


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

int main(int argc, char * argv[]) {
/*
    struct Profile* getProfile(){
        struct utsname uts;
        uname(&uts);

        struct Profile* outProf = (struct Profile*) malloc(sizeof(struct Profile));
        //architecture
        outProf->kernel = uts.sysname; //kernel name
        outProf->kernel_rel = uts.release; //kernel release
        outProf->kernel_v = uts.version; //kernel version
        outProf->hardware = uts.machine; //machine hardware
    }

    printf("Kernel: %s\n", getProfile()->kernel); 
    printf("Kernel Release: %s\n", getProfile()->kernel_rel);
    printf("Kernel Version: %s\n", getProfile()->kernel_v);
    printf("Hardware: %s\n", getProfile()->hardware);
    system("ip -4 addr show | grep -oP '(?<=inet\s)\d+(\.\d+){3}'");
    system("ip -6 addr show | grep -oP '(?<=inet\s)\d+(\.\d+){3}'");
    //ned ipinterfacename
    printf("Ethernet Address: \n"); system("hostname -I");
    printf("Gateway Router Address: \n"); system("ip route | grep default"); //gatewayrouteraddr
    printf("Name: \n"); system("id -un"); //name
    printf("UID: \n"); system("id -u"); //uid
    printf("GID: \n"); system("id -g"); //gid
    printf("Keyboard: \n");  system("localectl status"); //keyboard
    */
    char * outSTR = malloc(2048);
    outSTR= strProfile();
    printf("%s", outSTR);
    return 0;


}

