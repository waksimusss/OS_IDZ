#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define PORT 8888

sig_atomic_t killed = 0;
void killing_handler(int sig) {
    killed = 1;
}

int main(int argc, char** argv) {
    int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = { 0 };
    if (argc == 2) {
        addr.sin_addr.s_addr = inet_addr(argv[0]);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (argc == 2) {
        int port;
        sscanf(argv[1], "%d", &port);
        addr.sin_port = htons(port);
    }

    int connect_res = connect(fd_socket, (struct sockaddr*)&addr, sizeof addr);
    if (connect_res == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    ssize_t nread;
    signal(SIGINT, killing_handler);
    char buf[256];

    while (1) {
        if (killed) {
            printf("Клиент дисплея закрыт\n");
            write(STDOUT_FILENO, buf, nread);
            close(fd_socket);
            exit(0);
        }

        nread = read(fd_socket, buf, 256);
        if (nread == -1) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        if (nread == 0) {
            printf("END OF FILE occured\n");
        }

        printf("%s\n", buf);
    }

    return 0;
}
