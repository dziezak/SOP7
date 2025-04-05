#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define SHM_NAME "/semaphore_shm"
#define SEM_NAME "/semaphore"
#define SHM_SIZE sizeof(SharedMemory)

typedef struct{
    char buffer[5][1024];
    int in;
    int out;
} SharedMemory;

sem_t *sem;
int fd;
SharedMemory *data;

void handle_sigint(int sig){
    munmap(data, SHM_SIZE);
    close(fd);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
    printf("\nProgram zakocznony poprawnie po SGINT.\n");
    exit(0);
}

void sleep_random(){
    int delay = rand() % 3;
    sleep(delay);
}

int main(){
    signal(SIGINT, handle_sigint);
    srand(time(NULL));
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666); 
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    ftruncate(fd, SHM_SIZE);
    data = (SharedMemory*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(data == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if(sem == SEM_FAILED){
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    data->in = 0;
    data->out = 0;

    pid_t pid = fork();
    if(pid == 0){
        while(1){
            sem_wait(sem);
            if(data->in != data->out){
                printf("Konsument dosatl wiadomosc: %s\n", data->buffer[data->out]);
                data->out = (data->out + 1) % 5;
            }else{
                printf("Kosumnet: Brak danych od odbioru.\n");
            }
            sem_post(sem);
            sleep_random();
        }
        exit(0);
    }else{
        for(int i=0; i<10; i++){
            sem_wait(sem);
            if((data->in+1) % 5 != data->out){
                snprintf(data->buffer[data->in], sizeof(data->buffer[0]), "Wiadomosc nr %d", i);
                printf("Producent: Zapisano wiadomosc: Wiadomosc nr %d\n", i);
                data->in = (data->in+1)%5;
            }else{
                printf("Producent: Buffor pelny, czekam na miejsce...\n");
            }
            sem_post(sem);
            sleep_random();
        }
        
        snprintf(data->buffer[data->in], sizeof(data->buffer[0]), "witaj");
        data->in = (data->in + 1) % 5;
        printf("Producent: Dane zapisane do pamieci dzielonej\n");

        sem_post(sem);
        wait(NULL);

        munmap(data, SHM_SIZE);
        close(fd);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        shm_unlink(SHM_NAME);
    }
    return 0;
}