#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SOCKET_NAME "mysocket.s"
#define BUF_SIZE 256
#define OK 0

int main(void)
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    struct sockaddr srvr_name;
    srvr_name.sa_family = AF_UNIX;
    strcpy(srvr_name.sa_data, SOCKET_NAME);

    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "My pid is: %d", getpid());

    if(connect(sockfd, &srvr_name,
               strlen(srvr_name.sa_data) + 1 + sizeof(srvr_name.sa_family)) < 0)
    {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    send(sockfd, buf, strlen(buf) + 1, 0);
    printf("Client sent: %s\n", buf);

    recv(sockfd, buf, BUF_SIZE, 0);
    printf("Client got: %s\n", buf);

    close(sockfd);
    return OK;
}