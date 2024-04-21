#include <stdlib.h> 
#include <time.h>    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_SIZE 100

int main() {
    srand(time(NULL));

    key_t shmkey = ftok(".", 'a');
    key_t semkey = ftok(".", 'b');

    int shmid = shmget(shmkey, sizeof(int) * MAX_SIZE, IPC_CREAT | 0666);
    int* data = (int*)shmat(shmid, NULL, 0);
    if (data == (int*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    int semid = semget(semkey, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int index = rand() % MAX_SIZE;
        int old_value = data[index];
        int new_value = rand() % 1000;

        struct sembuf sem_lock = {0, -1, 0};
        struct sembuf sem_unlock = {0, 1, 0};

        semop(semid, &sem_lock, 1);
        data[index] = new_value;
        printf("Writer %d: Changed index %d from %d to %d\n", getpid(), index, old_value, new_value);
        semop(semid, &sem_unlock, 1);
        sleep(1);
    }

    shmdt(data);
    return 0;
}
