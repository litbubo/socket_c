#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     10

struct pollfd mypoll[1024];

int main()
{
    int sfd, cfd;
    int ret;
    int len;
    int i, max;
    socklen_t socklen;
    char ip[BUFSIZE];
    char buf[BUFSIZE];
    struct sockaddr_in saddr, caddr;

    memset(ip, 0, BUFSIZE);
    memset(buf, 0, BUFSIZE);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        perror("socket");
        exit(1);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVERIP, &saddr.sin_addr);
    ret = bind(sfd, (void *)&saddr, sizeof(saddr));
    if (ret < 0)
    {
        perror("bind");
        exit(1);
    }

    ret = listen(sfd, 128);
    if (ret < 0)
    {
        perror("listen");
        exit(1);
    }

    socklen = sizeof(caddr);

    for (i = 0; i < 1024; i++)
    {
        mypoll[i].fd = -1;
        mypoll[i].events = POLLIN;
    }
    mypoll[0].fd = sfd;
    max = 0;

    while (1)
    {
        ret = poll(mypoll, max + 1, -1);
        if (ret < 0)
        {
            perror("poll");
            exit(1);
        }
        if (mypoll[0].revents & POLLIN)
        {
            cfd = accept(sfd, (void *)&caddr, &socklen);
            if (cfd < 0)
            {
                perror("accept");
                exit(1);
            }
            fprintf(stdout, "[%s]:%d connect ...\n",
                    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
                    ntohs(caddr.sin_port));
            for (i = 0; i < 1024; i++)
            {
                if (mypoll[i].fd == -1)
                {
                    mypoll[i].fd = cfd;
                    max = i > max ? i : max;
                    break;
                }
            }
            ret--;
        }
        for (i = 1; i < 1024 && ret > 0; i++)
        {
            if (mypoll[i].revents & POLLIN)
            {
                memset(buf, 0, BUFSIZE);
                len = recv(mypoll[i].fd, buf, sizeof(buf) - 1, 0);
                buf[len] = 0;
                if (len == 0)
                {
                    fprintf(stdout, "data is end...\n");
                    close(mypoll[i].fd);
                    mypoll[i].fd = -1;
                }
                else if (len < 0)
                {
                    perror("recv");
                    close(mypoll[i].fd);
                    mypoll[i].fd = -1;
                }
                else
                {
                    fprintf(stdout, "%s", buf);
                    for (int i = 0; i < len; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    send(mypoll[i].fd, buf, len, 0);
                }
                ret--;
            }
        }
    }
    close(sfd);
    exit(0);
}
