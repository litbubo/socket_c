#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT        30001
#define SERVERIP    "127.0.0.1"
#define BUFSIZE     1024

int main()
{
    int fd;
    int ret;
    char buf[BUFSIZE];
    struct sockaddr_in saddr;
    memset(buf, 0, sizeof(buf));
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        perror("socket");
        exit(1);
    }
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVERIP, &saddr.sin_addr);
    ret = connect(fd, (void *)&saddr, sizeof(saddr));
    if(ret < 0)
    {
        perror("connect");
        exit(1);
    }
    while(1)
    {
        memset(buf, 0, BUFSIZE);
        fgets(buf, BUFSIZE, stdin);
        fflush(NULL);
        send(fd, buf, strlen(buf) + 1, 0);
        memset(buf, 0, BUFSIZE);
        recv(fd, buf, BUFSIZE, 0);
        fprintf(stdout, "%s", buf);
        fflush(NULL);
    }

    close(fd);
    printf("Hello world\n");
    return 0;
}

