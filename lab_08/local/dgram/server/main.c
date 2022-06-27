#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

#define SOCKET_NAME "mysocket.s"
#define BUF_SIZE 256
#define OK 0

static int sockfd;

void cleanup_socket(void)
{
    close(sockfd);
    unlink(SOCKET_NAME);
}

void sigint_handler(int signum)
{
    cleanup_socket();
    fprintf(stdout, "Server stopped listening\n");
    exit(OK);
}

int main(void)
{
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    struct sockaddr from;
    unsigned fromlen;

    struct sockaddr to;
    to.sa_family = AF_UNIX;
    strcpy(to.sa_data, SOCKET_NAME);
    if (bind(sockfd, &to, strlen(to.sa_data) + 1 + sizeof(to.sa_family)) < 0)
    {
        perror("Failed to bind socket");
        return EXIT_FAILURE;
    }

    signal(SIGINT, sigint_handler);
    fprintf(stdout, "Server is listening.\nTo stop server press Ctrl + C.\n");
    char buf[BUF_SIZE];
    for (;;)
    {
        int bytes = recvfrom(sockfd, buf, sizeof(buf), 0, &from, &fromlen);
        if (bytes <= 0) {
            perror("Failed to recv");
            cleanup_socket();
            return EXIT_FAILURE;
        }
        fprintf(stdout, "Server got: %s\n", buf);

        snprintf(buf, BUF_SIZE, "Server: my pid is: %d", getpid());
        int a = sendto(sockfd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&from, fromlen);
        fprintf(stdout, "Server sent: %s\n", buf);
    }

}