#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <semaphore.h>

char fd1_name[] = "fd1"; // первый именованный канал
char fd2_name[] = "fd2"; // второй именованный канал

const char *sem_name = "/full-semaphore";
sem_t *sem;

#define BUF_SIZE 5000