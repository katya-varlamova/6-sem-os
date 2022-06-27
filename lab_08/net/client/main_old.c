
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 256
#define SOCKET_ADDR "localhost"
#define SOCKET_PORT 9999

#define OK 0

int main(void)
{
    const int master_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (master_sd == -1) {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(SOCKET_PORT)
    };
    if (connect(master_sd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        perror("Failed to connect");
        return EXIT_FAILURE;
    }
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "My pid is %d", getpid());
    if (sendto(master_sd, msg, strlen(msg), 0, (struct sockaddr *) &addr, sizeof addr) < 0) {
        perror("Failed to sendto");
        return EXIT_FAILURE;
    }
    close(master_sd);
    return 0;
}