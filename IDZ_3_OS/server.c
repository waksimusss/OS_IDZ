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
#define PORT 8888
#define MAX_CLIENTS 3

void swap(int* xp, int* yp) {
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}

void selectionSort(int arr[]) {
	int min_idx;

	for (int i = 0; i < LIBRARY_SIZE - 1; ++i) {
		min_idx = i;
		for (int j = i + 1; j < LIBRARY_SIZE; ++j) {
			if (arr[j] < arr[min_idx]) {
				min_idx = j;
			}
		}

		swap(&arr[min_idx], &arr[i]);
	}
}

void join(int* arr, char* buffer) {
	int offset = 0;
	for (int i = 0; i < LIBRARY_SIZE; ++i) {
		int length = snprintf(NULL, 0, "%d", arr[i]);
		char temp[length + 1]; 
		snprintf(temp, length + 1, "%d", arr[i]); 
		if (i > 0) {
			buffer[offset] = ' '; 
			++offset;
		}
		for (int j = 0; j < length; ++j) {
			buffer[offset] = temp[j]; 
			++offset;
		}
	}
	buffer[offset] = '\n';
	buffer[offset + 1] = '\0';
}

int library[LIBRARY_SIZE];

sig_atomic_t killed = 0;
void killing_handler(int sig) {
	killed = 1;
}

int main(int argc, char** argv) {
	int client_socket[MAX_CLIENTS];
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		client_socket[i] = 0;
	}

	int master_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (master_socket == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) == -1) {
		perror("setsockopt failed");
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

	int bind_res = bind(master_socket, (struct sockaddr *) & addr, sizeof addr);
	if (bind_res == -1) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	int listen_res = listen(master_socket, MAX_CLIENTS);
	if (listen_res == -1) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	socklen_t addrlen = sizeof addr;

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		client_socket[i] = accept(master_socket, (struct sockaddr*)&addr, &addrlen);
		if (client_socket[i] == -1) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}
	}

	ssize_t nread;
	char buf[256];
	signal(SIGINT, killing_handler);

	for (int i = 0; i < LIBRARY_SIZE; ++i) {
		library[i] = i + 1;
	}

	sprintf(buf, "Library is initialized\n");
	write(client_socket[2], buf, 256);

	// i == 0: readers
	// i == 1: writers
	for (int i = 0; i < MAX_CLIENTS - 1; ++i) {
		if (fork() != 0) {
			continue;
		}		

		if (i == 0) {
			while (1) {
				if (killed) {
					for (int k = 0; k < MAX_CLIENTS; ++k) {
						close(client_socket[k]);
					}

					close(listen_res);
					close(master_socket);
					printf("Все клиенты и сервер закрыты\n");
					exit(0);
				}

				nread = read(client_socket[i], buf, 256);
				if (nread == -1) {
					perror("read failed");
					exit(EXIT_FAILURE);
				}
				if (nread == 0) {
					printf("END OF FILE occured\n");
					continue;
				}

				int reader;
				sscanf(buf, "%d", &reader);

				nread = read(client_socket[i], buf, 256);
				if (nread == -1) {
					perror("read failed");
					exit(EXIT_FAILURE);
				}
				if (nread == 0) {
					printf("END OF FILE occured\n");
					continue;
				}

				int book;
				sscanf(buf, "%d", &book);

				sprintf(buf, "Reader #%d read the book #%d. Result: %d * %d = %d\n", 
					reader + 1, book + 1, book + 1, library[book], (book + 1) * (library[book]));
				write(client_socket[2], buf, sizeof(buf));

				join(library, buf);
				write(client_socket[2], buf, sizeof(buf));
			}
 		}
		else if (i == 1) {
			while (1) {
				if (killed) {
					for (int k = 0; k < MAX_CLIENTS; ++k) {
						close(client_socket[k]);
					}

					close(listen_res);
					close(master_socket);
					printf("Все клиенты и сервер закрыты\n");
					exit(0);
				}

				nread = read(client_socket[i], buf, 256);
				if (nread == -1) {
					perror("read failed");
					exit(EXIT_FAILURE);
				}
				if (nread == 0) {
					printf("END OF FILE occured\n");
					continue;
				}

				int writer;
				sscanf(buf, "%d", &writer);

				nread = read(client_socket[i], buf, 256);
				if (nread == -1) {
					perror("read failed");
					exit(EXIT_FAILURE);
				}
				if (nread == 0) {
					printf("END OF FILE occured\n");
					continue;
				}

				int book;
				sscanf(buf, "%d", &book);
				int new_book = rand() % 30;

				sprintf(buf, "Writer #%d written the book #%d. It was %d: now it's %d\n",
					writer + 1, book + 1, library[book], new_book);
				write(client_socket[2], buf, sizeof(buf));

				library[book] = new_book;
				selectionSort(library);

				join(library, buf);
				write(client_socket[2], buf, sizeof(buf));
			}
		}

		exit(0);
	}

	return 0;
}