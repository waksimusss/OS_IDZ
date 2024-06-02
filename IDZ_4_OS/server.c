#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

#define LIBRARY_SIZE 20
#define PORT 8882
#define MAX_CLIENTS 3
#define BUFFER_SIZE 256

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



sig_atomic_t killed = 0;
void killing_handler(int sig) {
	killed = 1;
}

int main(int argc, char** argv) {
	char buf[BUFFER_SIZE];
	signal(SIGINT, killing_handler);

	int *library;
	library = mmap(NULL, LIBRARY_SIZE * sizeof(int),
                  PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED,
                  -1, 0);

	int master_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (master_socket == -1) {
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

	int bind_res = bind(master_socket, (struct sockaddr *) & addr, addrlen);
	if (bind_res == -1) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in client_addr[MAX_CLIENTS];
	socklen_t client_len;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		memset(&client_addr[i], 0, sizeof(client_addr[i]));
		// Request to connect
		client_len = sizeof(client_addr[i]);
		recvfrom(master_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr[i], &client_len);
	} 

	for (int i = 0; i < LIBRARY_SIZE; ++i) {
		library[i] = i + 1;
	}

	sprintf(buf, "Library is initialized\n");
	sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[2]), client_len);

	// i == 0: readers
	// i == 1: writers
	sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[0]), client_len);
	sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[1]), client_len);
	for (int i = 0; i < MAX_CLIENTS - 1; ++i) {
		if (fork() != 0) {
			continue;
		}		

		if (i == 0) {
			while (1) {
				if (killed) {
					close(master_socket);
					exit(0);
				}

				recvfrom(master_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr[i], &client_len);
				int reader;
				sscanf(buf, "%d", &reader);

				recvfrom(master_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr[i], &client_len);
				int book;
				sscanf(buf, "%d", &book);

				sprintf(buf, "Reader #%d read the book #%d. Result: %d * %d = %d\n", 
					reader + 1, book + 1, book + 1, library[book], (book + 1) * (library[book]));
				sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[2]), client_len);
			}
 		}
		else if (i == 1) {
			while (1) {
				if (killed) {
					close(master_socket);
					exit(0);
				}

				recvfrom(master_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr[i], &client_len);
				int writer;
				sscanf(buf, "%d", &writer);

				recvfrom(master_socket, buf, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr[i], &client_len);
				int book;
				sscanf(buf, "%d", &book);
				int new_book = rand() % 30;

				sprintf(buf, "Writer #%d written the book #%d. It was %d: now it's %d\n",
					writer + 1, book + 1, library[book], new_book);
				sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[2]), client_len);

				library[book] = new_book;
				selectionSort(library);

				join(library, buf);
				sendto(master_socket, buf, BUFFER_SIZE, MSG_CONFIRM, (const struct sockaddr*)&(client_addr[2]), client_len);
			}
		}

		exit(0);
	}

	return 0;
}