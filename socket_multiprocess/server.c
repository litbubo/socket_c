#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     1024

void callback(int s)
{
    int ret;
    while(1)
    {
        ret = waitpid(-1, NULL, WNOHANG);
        if(ret <= 0)
        {
            break;
        }
        fprintf(stdout, "%d has exited...\n", ret);
    }
}

int main()
{
    int sfd, cfd;
    int ret;
    int len;
    int i;
    pid_t pid;
    socklen_t socklen;
    char ip[BUFSIZE];
    char buf[BUFSIZE];
    struct sockaddr_in saddr, caddr; 
    struct sigaction act;

    memset(ip, 0, BUFSIZE);
    memset(buf, 0, BUFSIZE);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0)
    {
        perror("socket");
        exit(1);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVERIP, &saddr.sin_addr);
    ret = bind(sfd, (void *)&saddr, sizeof(saddr));
    if(ret < 0)
    {
        perror("bind");
        exit(1);
    }

    ret = listen(sfd, 128);
    if(ret < 0)
    {
        perror("listen");
        exit(1);
    }
    socklen = sizeof(caddr);

    act.sa_handler = callback;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    while(1)
    {
        cfd = accept(sfd, (void *)&caddr, &socklen);
        if(cfd < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            perror("accept");
            exit(1);
        }
        pid = fork();
        if(pid < 0)
        {
            perror("fork");
            exit(1);
        }
        else if(pid == 0)
        {
            close(sfd);
            fprintf(stdout, "[%s]:%d connect ...\n", 
                    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)), 
                    ntohs(caddr.sin_port));
            while(1)
            {
                memset(buf, 0, BUFSIZE);
                len = recv(cfd, buf, sizeof(buf), 0);
                if(len == 0)
                {
                    fprintf(stdout, "data is end...\n");
                    close(cfd);
                    exit(0);
                }
                else if(len < 0)
                {
                    perror("recv");
                    close(cfd);
                    exit(1);
                }
                else
                {
                    fprintf(stdout, "%s", buf);
                    for(i = 0; i < len; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    send(cfd, buf, strlen(buf) + 1, 0);
                }
            }

        }
        else
        {
            close(cfd);
        }
    }
    close(sfd);
    exit(0);
}

