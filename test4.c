#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 5  // Rozmiar bufora

typedef struct {
    char buffer[BUFFER_SIZE][100];  // Kolejka FIFO
    int in;  // Wskaźnik zapisu
    int out; // Wskaźnik odczytu
    sem_t empty_slots;  // Ile miejsc wolnych
    sem_t filled_slots; // Ile miejsc zajętych
    pthread_mutex_t mutex; // Mutex do ochrony dostępu
} SharedMemory;

int main() {
    // Tworzymy współdzieloną pamięć
    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Inicjalizacja semaforów i mutexa
    sem_init(&shared_mem->empty_slots, 1, BUFFER_SIZE);
    sem_init(&shared_mem->filled_slots, 1, 0);
    pthread_mutex_init(&shared_mem->mutex, NULL);

    shared_mem->in = 0;
    shared_mem->out = 0;

    pid_t pid1 = fork();
    if (pid1 == 0) {  // Pierwszy producent
        for (int i = 0; i < 10; i++) {
            sem_wait(&shared_mem->empty_slots);  // Czekamy na wolne miejsce
            pthread_mutex_lock(&shared_mem->mutex);  

            // Wstawiamy wiadomość do bufora
            snprintf(shared_mem->buffer[shared_mem->in], 100, "Producent 1 -> %d", i);
            printf("Producent 1: Dodano %d\n", i);
            shared_mem->in = (shared_mem->in + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&shared_mem->mutex);
            sem_post(&shared_mem->filled_slots);  // Powiadamiamy, że coś jest w buforze

            sleep(rand() % 2);
        }
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {  // Drugi producent
        for (int i = 0; i < 10; i++) {
            sem_wait(&shared_mem->empty_slots);
            pthread_mutex_lock(&shared_mem->mutex);

            snprintf(shared_mem->buffer[shared_mem->in], 100, "Producent 2 -> %d", i);
            printf("Producent 2: Dodano %d\n", i);
            shared_mem->in = (shared_mem->in + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&shared_mem->mutex);
            sem_post(&shared_mem->filled_slots);

            sleep(rand() % 2);
        }
        exit(0);
    }

    pid_t pid3 = fork();
    if (pid3 == 0) {  // Konsument
        for (int i = 0; i < 20; i++) {  // Odbiera 20 wiadomości
            sem_wait(&shared_mem->filled_slots);  // Czekamy na dane
            pthread_mutex_lock(&shared_mem->mutex);

            printf("Konsument: Odebrano -> %s\n", shared_mem->buffer[shared_mem->out]);
            shared_mem->out = (shared_mem->out + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&shared_mem->mutex);
            sem_post(&shared_mem->empty_slots);  // Zwolnienie miejsca w buforze

            sleep(rand() % 3);
        }
        exit(0);
    }

    // Czekamy na zakończenie wszystkich procesów
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // Czyszczenie zasobów
    sem_destroy(&shared_mem->empty_slots);
    sem_destroy(&shared_mem->filled_slots);
    pthread_mutex_destroy(&shared_mem->mutex);
    munmap(shared_mem, sizeof(SharedMemory));

    return 0;
}
