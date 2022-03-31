#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     11

int main()
{
    int sfd, cfd, maxfd;
    int ret;
    int len;
    int i;
    socklen_t socklen;
    fd_set rdset, tmpset;
    struct sockaddr_in saddr, caddr;
    char buf[BUFSIZE];
    char ip[BUFSIZE];

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

    maxfd = sfd;
    FD_ZERO(&rdset);
    FD_SET(sfd, &rdset);

    while (1)
    {
        tmpset = rdset;
        ret = select(maxfd + 1, &tmpset, NULL, NULL, NULL);
        if (FD_ISSET(sfd, &tmpset))
        {
            socklen = sizeof(caddr);
            cfd = accept(sfd, (void *)&caddr, &socklen);
            if (cfd < 0)
            {
                perror("accept");
                exit(1);
            }
            fprintf(stdout, "[%s]:%d connect ...\n",
                    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, ip, sizeof(ip)),
                    ntohs(caddr.sin_port));
            FD_SET(cfd, &rdset);
            maxfd = cfd > sfd ? cfd : maxfd;
            ret--;
        }
        for (i = 0; i < maxfd + 1 && ret > 0; i++)
        {
            if (i != sfd && FD_ISSET(i, &tmpset))
            {
                memset(buf, 0, BUFSIZE);
                len = recv(i, buf, sizeof(buf) - 1, 0);
                buf[len] = 0;
                if (len == 0)
                {
                    fprintf(stdout, "data is end...\n");
                    FD_CLR(i, &rdset);
                    close(i);
                }
                else if (len < 0)
                {
                    perror("recv");
                    FD_CLR(i, &rdset);
                    close(i);
                }
                else
                {
                    fprintf(stdout, "%s", buf);
                    for (int i = 0; i < len; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    send(i, buf, len, 0);
                }
                ret--;
            }
        }
    }
    close(sfd);
    exit(0);
}
