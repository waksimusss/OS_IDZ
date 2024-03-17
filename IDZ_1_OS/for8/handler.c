#include "common.h"

// функция, которая ищет индексы подпоследовательности
char * find(char buffer[], char string[]) {
    int len_str = strlen(buffer);
    int len_substr = strlen(string);
    int index = 0;
    char* result = malloc(sizeof(len_str));
    for (int i = 0; i <= len_str - len_substr; i++) {
        int j;
        for (j = 0; j < len_substr; j++) {
            if (buffer[i + j] != string[j]) {
                break;
            }
        }
        if (j == len_substr) {
            if (i >= 10) {
                result[index++] = '0' + i / 10; 
                result[index++] = '0' + i % 10; 
            } else {
                result[index++] = '0' + i; 
            }
            if (i != len_substr - 1) {
                result[index++] = ' ';
            }
        }
    }
    return result;
}

//функция для разделения буфера на строку и подстроку
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}

int main(int argc, char *argv[]) {
    char input_filename[256];  // имя файла для чтения
    char output_filename[256]; // имя файла для записи

    int input_descriptor;
    int output_descriptor;

    int fd1;
    int fd2;

    char buffer[BUF_SIZE]; // буфер

    (void)umask(0);

    if ((sem = sem_open(sem_name, O_CREAT, 0666, 0)) == 0) {
        perror("Reader: can't create admin semaphore");
        exit(10);
    }

    if (access(fd1_name, F_OK) == -1) {
        if (mknod(fd1_name, S_IFIFO | 0666, 0) < 0) {
            printf("->Reader: error while creating channel 1<-\n");
            exit(10);
        }
    }

    if (access(fd2_name, F_OK) == -1) {
        if (mknod(fd2_name, S_IFIFO | 0666, 0) < 0) {
            printf("->Reader: error while creating channel 2<-\n");
            exit(10);
        }
    }

    if ((fd1 = open(fd1_name, O_RDONLY)) < 0) { // открытие первого канала
        printf("->Handler: error with open FIFO 1 for reading<-\n");
        exit(10);
    }

    int size = read(fd1, buffer, BUF_SIZE);
    if (size < 0) {
        printf("->Handler: error with reading from pipe 1<-\n");
        exit(10);
    }

    char* res = find(str_split(buffer, '\n')[0], str_split(buffer, '\n')[1]);
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, res, sizeof(res));// обработка текста

    if (close(fd1) != 0) { // закрытие канала
        printf("->Reader: error with closing fifo 1<-\n");
        exit(10);
    }

    if ((fd2 = open(fd2_name, O_WRONLY)) < 0) { // открытие второго канала
        printf("->Handler: error with open FIFO 1 for reading<-\n");
        exit(10);
    }

    size = write(fd2, buffer, size); // запись текста из буфера в канал
    if (size < 0) {
        printf("->Handler: error with writing<-\n");
        exit(10);
    }

    if (close(fd2) < 0) {
        printf("->Reader: error with closing fifo 2<-\n");
        exit(10);
    }

    printf("->Handler: exit<-\n");

    if (sem_post(sem) == -1) {
        printf("Handler: can't increment admin semaphore");
        exit(10);
    }

    if (sem_close(sem) == -1) {
        printf("Handler: incorrect close of busy semaphore");
        exit(10);
    }

    return 0;
}