#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>


#define BUFFER_SIZE 5
#define SHM_NAME "/semaphore_shm"
#define PRODUCENT_NUMER 3
#define CONSUMENT_NUMBER 2
#define NUMBER_OF_DOCUMENTS 2
#define SHM_SIZE sizeof(SharedMemory)

typedef struct{
    char buffer[BUFFER_SIZE][1024];
    int in;
    int out;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
} SharedMemory;

void sleep_random(){
    int delay = rand() % 3;
    sleep(delay);
}

int main(){
    int fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, 0666);
    if(fd == -1){
        perror("shm_open");
        exit(1);
    }

    ftruncate(fd, SHM_SIZE);
    SharedMemory *data = (SharedMemory*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(data == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    sem_init(&data->empty, 1, BUFFER_SIZE);
    sem_init(&data->full, 1, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->mutex, &attr);


    data->in = 0;
    data->out = 0;

    for(int i=0; i<PRODUCENT_NUMER; i++){
        int pid = fork();
        if(pid == 0){
            srand(time(NULL)^getpid());
            for(int j=0; j<NUMBER_OF_DOCUMENTS; j++){
                sem_wait(&data->empty);
                pthread_mutex_lock(&data->mutex);

                snprintf(data->buffer[data->in], sizeof(data->buffer[0]), "Dokument %d od producenta %d",j, i);
                printf("Producent %d: Dodano -> %s\n", i, data->buffer[data->in]);

                data->in = (data->in + 1) % BUFFER_SIZE;
                
                pthread_mutex_unlock(&data->mutex);
                sem_post(&data->full);
                sleep_random();
            }
            exit(0);
        }
    }

    for(int i=0; i<CONSUMENT_NUMBER; i++){
        int pid = fork();
        if(pid == 0){
            for(int j=0; j<NUMBER_OF_DOCUMENTS*PRODUCENT_NUMER/CONSUMENT_NUMBER; j++){
                sem_wait(&data->full);
                pthread_mutex_lock(&data->mutex);

                printf("Konsument %i: Odebrano -> %s\n", i, data->buffer[data->out]);

                data->out = (data->out + 1) % BUFFER_SIZE;

                pthread_mutex_unlock(&data->mutex);
                sem_post(&data->empty);
                sleep_random();
            }
            exit(0);
        }
    }

    for(int i=0; i<CONSUMENT_NUMBER+PRODUCENT_NUMER; i++){
        wait(NULL);
    }

    sem_destroy(&data->empty);
    sem_destroy(&data->full);
    pthread_mutex_destroy(&data->mutex);
    munmap(data, SHM_SIZE);
    shm_unlink(SHM_NAME);
    close(fd);
    return 0;
}