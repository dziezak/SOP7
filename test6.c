#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#define SHM_NAME "/semaphore_shm"
#define SEM_NAME "/semaphore"
#define SHM_SIZE 1024

int main(){
    int fd = open(SHM_NAME, O_CREAT|O_RDWR, 0666);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    ftruncate(fd, SHM_SIZE);
    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shm_ptr == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if(sem == SEM_FAILED){
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if(pid == 0){
        sem_wait(sem);
        printf("Konsument: odczytano -> %s\n", shm_ptr);
        munmap(shm_ptr, SHM_SIZE);
        close(fd);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        shm_unlink(SHM_NAME);
        exit(0);
    }else{
        strcpy(shm_ptr, "Dane od producenta!");
        printf("Producent: Dane zapisane do pamieci dzielonej.\n");
        sem_post(sem);
        wait(NULL);
    }

    munmap(shm_ptr, SHM_SIZE);
    close(fd);
    sem_close(sem);
    shm_unlink(SHM_NAME);
    return 0;
}