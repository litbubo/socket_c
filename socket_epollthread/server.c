#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     10

typedef struct SockInfo
{
    int fd;
    int epfd;
}SockInfo_t;

void *working(void *arg)
{
    SockInfo_t *sock = (SockInfo_t *)arg;
    char buf[BUFSIZE];
    int len, i;
    memset(buf, 0, BUFSIZE);
    int flag = fcntl(sock->fd, F_GETFL);
    flag |= O_NONBLOCK;                                                        
    fcntl(sock->fd, F_SETFL, flag);
    while(1)
    {
        len = recv(sock->fd, buf, sizeof(buf) - 1, 0);
        buf[len] = 0;
        if (len == 0)
        {
            fprintf(stdout, "data is end...\n");
            epoll_ctl(sock->epfd, EPOLL_CTL_DEL, sock->fd, NULL);
            close(sock->fd);
            break;
        }
        else if (len < 0)
        {
            if(errno == EAGAIN)
                break;
            perror("recv");
            epoll_ctl(sock->epfd, EPOLL_CTL_DEL, sock->fd, NULL);
            close(sock->fd);
            break;
        }
        else
        {
            fprintf(stdout, "%s", buf);
            for (i = 0; i < len; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            send(sock->fd, buf, len, 0);
        }
    }
    free(sock);
    pthread_exit(NULL);
}

int main()
{
    int sfd, fd, epfd, cfd;
    int ret;
    int i;
    socklen_t socklen;
    char ip[BUFSIZE];
    struct sockaddr_in saddr, caddr;
    struct epoll_event ev;

    memset(ip, 0, BUFSIZE);

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

    epfd = epoll_create(100);
    if(epfd < 0)
    {
        perror("epoll_create");
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    struct epoll_event evt[1024];

    while (1)
    {

        ret = epoll_wait(epfd, evt, 1024, -1);
        for(i = 0; i < ret; i++)
        {
            fd = evt[i].data.fd;
            if(fd == sfd)
            {
                cfd = accept(fd, (void *)&caddr, &socklen);
                if (cfd < 0)
                {
                    perror("accept");
                    exit(1);
                }
                fprintf(stdout, "[%s]:%d connect ...\n",
                        inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
                        ntohs(caddr.sin_port));
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            }
            else
            { 
                pthread_t tid;
                SockInfo_t *sock = malloc(sizeof(SockInfo_t));
                sock->fd = fd;
                sock->epfd = epfd;
                pthread_create(&tid, NULL, working, sock);
                pthread_detach(tid);
            }
        }
    }
    close(sfd);
    exit(0);
}
