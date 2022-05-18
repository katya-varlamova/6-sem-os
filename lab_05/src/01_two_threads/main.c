#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
void thread(int fd)
{
    FILE *fs = fdopen(fd, "r");
    char buff[20];
    setvbuf(fs, buff, _IOFBF, 20);

    char c;
    int flag = fscanf(fs, "%c", &c);
    while (flag == 1) {
        fprintf(stdout, "%c", c);
        flag = fscanf(fs, "%c", &c);
    }
}
int main() {
    int fd = open("alphabet.txt", O_RDONLY);

    pthread_t tid[2];
    for (int i = 0; i < 2; i++) {
        if (pthread_create(&tid[i], NULL, thread, fd)) {
            printf("Error: can't create thread\n");
            return -1; }
    }
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    return 0;
}