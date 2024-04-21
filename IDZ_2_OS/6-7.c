#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define MAX_SIZE 100
#define NUM_READERS 5
#define NUM_WRITERS 3

int* data;
sem_t* mutex;
sem_t* readers_count[NUM_READERS];
sem_t* writers_count[NUM_WRITERS];
char* mutex_name = "/mu";
char* readers_names[NUM_READERS] = {"/r0", "/r1", "/r2", "/r3", "/r4"};
char* writers_names[NUM_WRITERS] = {"/w0", "/w1", "/w2"};
int shm_id;

void* writer(void* arg) {
    intptr_t index = (intptr_t)arg;
    int rand_index = rand() % MAX_SIZE;
    int old_value = data[rand_index];
    int new_value = rand() % 1000;

    sem_wait(mutex); 
    data[rand_index] = new_value;
    printf("Writer %ld: Changed index %d from %d to %d\n", pthread_self(), rand_index, old_value, new_value);
    sem_post(mutex);
    sem_post(writers_count[index]);

    return NULL;
}

void* reader(void* arg) {
    intptr_t index = (intptr_t)arg;
    int rand_index = rand() % MAX_SIZE;
    int value = data[rand_index];
    int product = value * rand_index;

    sem_wait(writers_count[index]);
    sem_wait(mutex);
    printf("Reader %ld: Read index %d, value %d, product %d\n", pthread_self(), rand_index, value, product);
    sem_post(mutex);
    sem_post(readers_count[index]);

    return NULL;
}

void cleanup() {
    sem_unlink(mutex_name);
    sem_close(mutex);
    for (int i = 0; i < NUM_READERS; i++) {
        sem_unlink(readers_names[i]);
        sem_close(readers_count[i]);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        sem_unlink(writers_names[i]);
        sem_close(writers_count[i]);
    }
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
    shm_id = shmget(key, sizeof(int) * MAX_SIZE, IPC_CREAT | 0666);
    data = (int*)shmat(shm_id, NULL, 0);
    
    data[0] = 1;
    for (int i = 1; i < MAX_SIZE; i++) {
        data[i] = data[i-1]+rand()%1000;
    }

    mutex = sem_open(mutex_name, O_CREAT | O_EXCL, 0666, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < NUM_READERS; i++) {
        readers_count[i] = sem_open(readers_names[i], O_CREAT | O_EXCL, 0666, 0);
        if (readers_count[i] == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        writers_count[i] = sem_open(writers_names[i], O_CREAT | O_EXCL, 0666, 1);
        if (writers_count[i] == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, sigint_handler);

    pthread_t writer_threads[NUM_WRITERS], reader_threads[NUM_READERS];

    for (intptr_t i = 0; i < NUM_WRITERS; i++) {
        pthread_create(&writer_threads[i], NULL, writer, (void*)i);
    }

    for (intptr_t i = 0; i < NUM_READERS; i++) {
        pthread_create(&reader_threads[i], NULL, reader, (void*)i);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writer_threads[i], NULL);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    cleanup();

    return 0;
}
