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

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     10

int main()
{
    int sfd, fd, epfd, tmpfd;
    int ret;
    int len;
    int i;
    socklen_t socklen;
    char ip[BUFSIZE];
    char buf[BUFSIZE];
    struct sockaddr_in saddr, caddr;
    struct epoll_event ev;

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

    epfd = epoll_create(100);
    if (epfd < 0)
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
        for (i = 0; i < ret; i++)
        {
            fd = evt[i].data.fd;
            if (fd == sfd)
            {
                tmpfd = accept(fd, (void *)&caddr, &socklen);
                if (tmpfd < 0)
                {
                    perror("accept");
                    exit(1);
                }
                fprintf(stdout, "[%s]:%d connect ...\n",
                        inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
                        ntohs(caddr.sin_port));
                ev.events = EPOLLIN;
                ev.data.fd = tmpfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, tmpfd, &ev);
            }
            else
            {
                memset(buf, 0, BUFSIZE);
                len = recv(fd, buf, sizeof(buf) - 1, 0);
                buf[len] = 0;
                if (len == 0)
                {
                    fprintf(stdout, "data is end...\n");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
                else if (len < 0)
                {
                    perror("recv");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
                else
                {
                    fprintf(stdout, "%s", buf);
                    for (i = 0; i < len; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    send(fd, buf, len, 0);
                }
            }
        }
    }
    close(sfd);
    exit(0);
}
