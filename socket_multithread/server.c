#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     1024

typedef struct SockInfo
{
    int fd;
    pthread_t tid;
    struct sockaddr_in addr;
}SockInfo_t;

SockInfo_t Info[128];

void *working(void *arg)
{
    SockInfo_t *sock = (SockInfo_t *)arg;
    int len, i;
    char ip[BUFSIZE];
    char buf[BUFSIZE];
    memset(ip, 0, BUFSIZE);

    fprintf(stdout, "[%s]:%d connect ...\n", 
            inet_ntop(AF_INET, &sock->addr.sin_addr.s_addr, ip, sizeof(ip)), 
            ntohs(sock->addr.sin_port));
    while(1)
    {
        memset(buf, 0, BUFSIZE);
        len = recv(sock->fd, buf, sizeof(buf), 0);
        if(len == 0)
        {
            fprintf(stdout, "data is end...\n");
            close(sock->fd);
            sock->fd = -1;
            break;
        }
        else if(len < 0)
        {
            perror("recv");
            close(sock->fd);
            sock->fd = -1;
            break;
        }
        else
        {
            fprintf(stdout, "%s", buf);
            for(i = 0; i < len; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            send(sock->fd, buf, strlen(buf) + 1, 0);
        }

    }
    pthread_exit(NULL);
}

int main()
{
    int sfd, cfd;
    int ret;
    int i;
    struct sockaddr_in saddr;
    socklen_t socklen;
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

    memset(Info, 0, sizeof(SockInfo_t) * 128);
    for(i = 0; i < 128; i++)
    {
        Info[i].fd = -1;
        Info[i].tid = -1;
    }

    socklen = sizeof(struct sockaddr_in);
    while(1)
    {
        SockInfo_t *sockpoint;
        for(i = 0; i < 128; i++)
        {
            if(Info[i].fd == -1)
            {
                sockpoint = &Info[i];
                break;
            }
        }
        cfd = accept(sfd, (void *)&sockpoint->addr, &socklen);
        if(cfd < 0)
        {
            perror("accept");
            exit(1);
        }
        sockpoint->fd = cfd;
        pthread_create(&sockpoint->tid, NULL, working, sockpoint);
        pthread_detach(sockpoint->tid);
    }
    close(sfd);
    exit(0);
}

