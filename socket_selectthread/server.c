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
#include <pthread.h>

#define PORT        30001
#define SERVERIP    "0.0.0.0"
#define BUFSIZE     1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct SockInfo
{
    int fd;
    int *maxfd;
    fd_set *rdset;
} SockInfo_t;

void *acception(void *arg)
{
    SockInfo_t *sock = (SockInfo_t *)arg;
    socklen_t socklen = sizeof(struct sockaddr_in);
    int cfd, max;
    struct sockaddr_in saddr;
    char ip[BUFSIZE];
    cfd = accept(sock->fd, (void *)&saddr, &socklen);
    if (cfd < 0)
    {
        perror("accept");
        exit(1);
    }
    fprintf(stdout, "[%s]:%d connect ...\n",
            inet_ntop(AF_INET, &saddr.sin_addr.s_addr, ip, sizeof(ip)),
            ntohs(saddr.sin_port));
    pthread_mutex_lock(&mutex);
    FD_SET(cfd, sock->rdset);
    max = *(sock->maxfd);
    max = cfd > max ? cfd : max;
    *(sock->maxfd) = max;
    pthread_mutex_unlock(&mutex);
    free(sock);
    pthread_exit(NULL);
}

void *communication(void *arg)
{
    SockInfo_t *sock = (SockInfo_t *)arg;
    int len, i;
    fprintf(stdout, "tid == %ld is going to comm\n", pthread_self());
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    len = recv(sock->fd, buf, sizeof(buf) - 1, 0);
    buf[len] = 0;
    if (len == 0)
    {
        fprintf(stdout, "data is end...\n");
        pthread_mutex_lock(&mutex);
        FD_CLR(sock->fd, sock->rdset);
        pthread_mutex_unlock(&mutex);
        close(sock->fd);
    }
    else if (len < 0)
    {
        perror("recv");
        pthread_mutex_lock(&mutex);
        FD_CLR(sock->fd, sock->rdset);
        pthread_mutex_unlock(&mutex);
        close(sock->fd);
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
    free(sock);
    fprintf(stdout, "tid == %ld is going to end\n", pthread_self());
    pthread_exit(NULL);
}

int main()
{
    int sfd, maxfd;
    int ret;
    int i;
    struct sockaddr_in saddr;
    fd_set rdset, tmpset;

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
        pthread_mutex_lock(&mutex);
        tmpset = rdset;
        int maxtmp = maxfd;
        pthread_mutex_unlock(&mutex);
        ret = select(maxtmp + 1, &tmpset, NULL, NULL, NULL);
        if (FD_ISSET(sfd, &tmpset))
        {
            pthread_t tid;
            SockInfo_t *sock;
            sock = malloc(sizeof(SockInfo_t));
            sock->fd = sfd;
            sock->maxfd = &maxfd;
            sock->rdset = &rdset;
            pthread_create(&tid, NULL, acception, sock);
            pthread_detach(tid);
            ret--;
        }
        printf("------------------------\n");
        // for(i = 0; i < maxtmp + 1 && ret > 0; i++ )
        for (i = 0; i < 1024 + 1; i++)
        {
            if (i != sfd && FD_ISSET(i, &tmpset))
            {
                pthread_t tid;
                SockInfo_t *sock;
                sock = malloc(sizeof(SockInfo_t));
                sock->fd = i;
                sock->maxfd = &maxfd;
                sock->rdset = &rdset;
                fprintf(stdout, "%d thread is going to create...\n", i);
                pthread_create(&tid, NULL, communication, sock);
                pthread_detach(tid);
                // ret --;
            }
        }
    }
    close(sfd);
    exit(0);
}
