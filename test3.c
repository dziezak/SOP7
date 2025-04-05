#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SHARED_MEMORY_SIZE 1024

typedef struct {
    char message[SHARED_MEMORY_SIZE];
    sem_t sem_to_consumer;  // Producent -> Konsument
    sem_t sem_to_producer;  // Konsument -> Producent
} SharedMemory;

int main(){
    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if(shared_mem == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    // Inicjalizujemy semafory
    sem_init(&shared_mem->sem_to_consumer, 1, 0); // Konsument czeka
    sem_init(&shared_mem->sem_to_producer, 1, 0); // Producent czeka na odpowiedź

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        exit(1);
    }

    if(pid == 0){ // Konsument
        printf("Konsument czeka na wiadomość...\n");

        sem_wait(&shared_mem->sem_to_consumer); // Czeka aż producent coś zapisze
        printf("Konsument odebrał wiadomość: %s\n", shared_mem->message);

        // Tworzy odpowiedź
        char response[SHARED_MEMORY_SIZE];
        snprintf(response, sizeof(response), "Hello %s", shared_mem->message);
        strcpy(shared_mem->message, response);

        sem_post(&shared_mem->sem_to_producer); // Daje znać producentowi, że odpowiedź gotowa

        munmap(shared_mem, sizeof(SharedMemory));
        exit(0);
    } else { // Producent
        strcpy(shared_mem->message, "WABA LUBBA DUB DUB");
        printf("Producent: Wiadomość zapisana do pamięci dzielonej.\n");

        sem_post(&shared_mem->sem_to_consumer); // Daje znać konsumentowi

        sem_wait(&shared_mem->sem_to_producer); // Czeka na odpowiedź

        printf("Producent otrzymał odpowiedź: %s\n", shared_mem->message);

        wait(NULL);
        sem_destroy(&shared_mem->sem_to_consumer);
        sem_destroy(&shared_mem->sem_to_producer);
        munmap(shared_mem, sizeof(SharedMemory));
    }

    return 0;
}
