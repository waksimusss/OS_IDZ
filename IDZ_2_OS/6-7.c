#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_SIZE 100
#define NUM_READERS 5
#define NUM_WRITERS 3


//Программа на 6-7
int* data;
sem_t* mutex;
sem_t* readers_count;
sem_t* writers_count;
int shm_id;

void* writer(void* arg) {
    int index = rand() % MAX_SIZE;
    int old_value = data[index];
    int new_value = rand() % 100;

    sem_wait(mutex);
    data[index] = new_value;
    printf("Writer %ld: Changed index %d from %d to %d\n", pthread_self(), index, old_value, new_value);
    sem_post(mutex);
    sem_post(writers_count);

    return NULL;
}

void* reader(void* arg) {
    int index = rand() % MAX_SIZE;
    int value = data[index];
    int product = value *  index;
    

    sem_wait(writers_count);
    sem_wait(mutex);
    printf("Reader %ld: Read index %d, value %d, product %d\n", pthread_self(), index, value, product);
    sem_post(mutex);
    sem_post(readers_count);

    return NULL;
}

void cleanup() {
    sem_destroy(mutex);
    sem_destroy(readers_count);
    sem_destroy(writers_count);
    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
}

void sigint_handler(int sig) {
    cleanup();
    exit(0);
}

int main() {
    srand(time(NULL));

    key_t key = ftok(".", 'a');
    shm_id = shmget(key, sizeof(int) * MAX_SIZE + sizeof(sem_t) * 3, IPC_CREAT | 0666);
    data = (int*)shmat(shm_id, NULL, 0);
    
    data[0] = 1;
    for (int i = 1; i < MAX_SIZE; i++) {
        data[i] = data[i-1]+rand()%1000;
    }

    mutex = (sem_t*)((char*)data + sizeof(int) * MAX_SIZE);
    readers_count = (sem_t*)((char*)data + sizeof(int) * MAX_SIZE + sizeof(sem_t));
    writers_count = (sem_t*)((char*)data + sizeof(int) * MAX_SIZE + 2 * sizeof(sem_t));

    sem_init(mutex, 0, 1);
    sem_init(readers_count, 0, 0);
    sem_init(writers_count, 0, 1);

    signal(SIGINT, sigint_handler);

    pthread_t writer_threads[NUM_WRITERS], reader_threads[NUM_READERS];

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_create(&writer_threads[i], NULL, writer, NULL);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_create(&reader_threads[i], NULL, reader, NULL);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writer_threads[i], NULL);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    cleanup();

    // Создание именованного канала
    mkfifo("fifo", 0666);

    int fifo_fd = open("fifo", O_RDWR);

    // Чтение данных из именованного канала
    char buffer[MAX_SIZE];
    int bytes_read;

    for (int i = 0; i < NUM_READERS; i++) {
        bytes_read = read(fifo_fd, buffer, MAX_SIZE);
        printf("Reader %d: Received data: %s\n", i+1, buffer);
    }

    // Запись данных в именованный канал
    for (int i = 0; i < NUM_WRITERS; i++) {
        sprintf(buffer, "Writer %d: Data %d", i+1, rand() % 100);
        write(fifo_fd, buffer, strlen(buffer)+1);
        printf("Writer %d: Sent data: %s\n", i+1, buffer);
    }

    // Закрытие именованного канала
    close(fifo_fd);

    // Удаление именованного канала
    unlink("fifo");

    return 0;
}