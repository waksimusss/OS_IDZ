#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUF_SIZE 5000

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
    if (argc != 3) {
        printf("->Incorrect number of arguments<-\n");
        exit(10);
    }
    
    char input_filename[256]; // имя файла для чтения
    char output_filename[256]; // имя файла для записи

    int input_descriptor;
    int output_descriptor;

    int fd1[2]; // первый неименованный канал
    int fd2[2]; // второй неименованный канал

    int process; // id первого процесса

    char buffer[BUF_SIZE]; // буфер

    strcpy(input_filename, argv[1]); // получение имен файлов из командной строки
    strcpy(output_filename, argv[2]);

    if (pipe(fd1) < 0) { // создание первого канала
        printf("->Reader: can't open pipe 1<-\n");
        exit(10);
    }

    if (pipe(fd2) < 0) { // создание второго канала
        printf("->Reader: can't open pipe 2<-\n");
        exit(10);
    }

    process = fork(); // первый форк
    if (process < 0) {
        printf("->Reader: can't fork process<-\n");
        exit(10);
    } else if (process > 0) { //parent
        // открытие файла для чтения
        if ((input_descriptor = open(input_filename, O_RDONLY)) == -1) {
            printf("->Error with opening input file\n");
            exit(10);
        }
        
        if (close(fd1[0]) < 0) {
            printf("->Reader: can't close reading side of pipe 1<-\n");
            exit(10);
        }

        int size = read(input_descriptor, buffer, BUF_SIZE); // считывание текста из файла в буфер
        if (size < 0) {
            printf("->Reader: error with reading file<-\n");
            exit(10);
        }

        size = write(fd1[1], buffer, size); // запись текста из буфера в канал
        if (size < 0) {
            printf("->Reader: error with writing in pipe 1<-\n");
            exit(10);
        }

        if (close(fd1[1]) < 0) {
            printf("->Reader: can't close writing side of pipe 1<-\n");
            exit(10);
        }

        // закрытие файла
        if (close(input_descriptor) != 0) {
            printf("->Error with closing input file<-\n");
            exit(10);
        }

        char other_buffer[BUF_SIZE]; // буфер, другой для верности

        if (close(fd2[1]) < 0) {
            printf("->Reader: can't close writing side of pipe 2<-\n");
            exit(10);
        }

        size = read(fd2[0], other_buffer, BUF_SIZE); // если fd2 пустой, то процесс блокируется
        if (size < 0) {
           printf("->Reader: error with reading from pipe 2<-\n");
            exit(10);
        }

        if (close(fd2[0]) < 0) {
            printf("->Reader: can't close reading side of pipe<-\n");
            exit(10);
        }

        // открытие файла для записи. Если его нет, он будет создан. Если он уже существует, содержимое удалится
        if ((output_descriptor = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
            printf("->Error with opening output file\n");
            exit(10);
        }

        size = write(output_descriptor, other_buffer, size);
        if (size < 0) {
            printf("->Reader: error with writing output file<-\n");
            exit(10);
        }

        // закрытие файла
        if (close(output_descriptor) != 0) {
            printf("->Error with closing output file<-\n");
            exit(10);
        }
        
        wait(NULL); // ожидание дочернего процесса
        printf("->Reader: exit<-\n");
    } else { // child
        if (close(fd1[1]) < 0) {
            printf("->Handler: can't close writing side of pipe 1<-\n");
            exit(10);
        }

        int size = read(fd1[0], buffer, BUF_SIZE); 
        if (size < 0) {
            printf("->Handler: error with reading from pipe 1<-\n");
            exit(10);
        }

        char* res = find(str_split(buffer, '\n')[0], str_split(buffer, '\n')[1]);
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, res, sizeof(res));// обработка текста

        if (close(fd1[0]) < 0) {
            printf("->Handler: can't close reading side of pipe 1<-\n");
            exit(10);
        }

        if (close(fd2[0]) < 0) {
            printf("->Handler: can't close reading side of pipe 2<-\n");
            exit(10);
        }

        size = write(fd2[1], buffer, size); // запись текста из буфера в канал
        if (size < 0) {
            printf("->Handler: error with writing<-\n");
            exit(10);
        }

        if (close(fd2[1]) < 0) {
            printf("->Handler: can't close writing side of pipe<-\n");
            exit(10);
        }

        printf("->Handler: exit<-\n");
    }

    return 0;
}