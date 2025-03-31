#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>

#define SHARED_MEMORY_SIZE 1024
#define SEM_NAME "/my_semaphore"

typedef struct {
    sem_t semaphore;
    char message[SHARED_MEMORY_SIZE];
} SharedMemory;

int main(){
    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, 
    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(shared_mem == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    if(sem_init(&shared_mem->semaphore, 1, 0) == -1){
        perror("sem_init");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        exit(1);
    }

    if(pid == 0){
        printf("Kondument czeka na wiadomosc...\n");
        sem_wait(&shared_mem->semaphore);

        printf("Konsument odebral wiadomosc: %s\n", shared_mem->message);

        sem_destroy(&shared_mem->semaphore);
        munmap(shared_mem, sizeof(SharedMemory));
    }else{
        sleep(1);
        strcpy(shared_mem->message, "Hello from Producer!");
        printf("Producent wyslal wiadomosc...\n");

        sem_post(&shared_mem->semaphore);
        wait(NULL);
        munmap(shared_mem, sizeof(SharedMemory));
    }
    return 0;
}