#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define LIBRARY_SIZE 20
#define PORT 8882
#define READERS_SIZE 5
#define BUFFER_SIZE 256

sig_atomic_t killed = 0;
void killing_handler(int sig) {
    killed = 1;
}

int main(int argc, char** argv) {
    signal(SIGINT, killing_handler);
    char buf[BUFFER_SIZE];

    int fd_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = { 0 };
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (argc == 2) {
        addr.sin_addr.s_addr = inet_addr(argv[0]);
        int port;
        sscanf(argv[1], "%d", &port);
        addr.sin_port = htons(port);
    }

    sendto(fd_socket, "1", BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr *) &addr, addrlen);
    recvfrom(fd_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr*)&addr, &addrlen);
    for (int i = 0; i < READERS_SIZE; ++i) {
        if (fork() == 0) {
            while (1) {
                if (killed) {
                    close(fd_socket);
                    exit(0);
                }
                
                // отправляет номер читателя
                sprintf(buf, "%d", i);
                sendto(fd_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr *) &addr, addrlen);

                // отправляет номер книги
                sprintf(buf, "%d", rand() % LIBRARY_SIZE);
                sendto(fd_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr *) &addr, addrlen);

                sleep(rand() % 2 + 2);
            }

            exit(0);
        }
    }

    return 0;
}
