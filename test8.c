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

#define SHM_NAME "/semaphore_shm"
#define SHM_SIZE sizeof(SharedMemory)
#define BUFFER_SIZE 5
#define TOTAL_MESSAGES 10

typedef struct {
    char buffer[BUFFER_SIZE][1024];
    int in;
    int out;
    sem_t sem_empty;
    sem_t sem_full;
    pthread_mutex_t sem_mutex;
} SharedMemory;

int fd;

void sleep_random(){
    int delay = rand() % 3;
    sleep(delay);
}

int main() {
    srand(time(NULL));

    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(fd, SHM_SIZE);

    SharedMemory *data = (SharedMemory *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Inicjalizacja semaforów i mutexa tylko raz (przez pierwszy proces)
    sem_init(&data->sem_empty, 1, BUFFER_SIZE);
    sem_init(&data->sem_full, 1, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->sem_mutex, &attr);

    data->in = 0;
    data->out = 0;

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Producent
        for (int i = 0; i < TOTAL_MESSAGES; i++) {
            sem_wait(&data->sem_empty);
            pthread_mutex_lock(&data->sem_mutex);

            snprintf(data->buffer[data->in], sizeof(data->buffer[0]), "Dokument %d", i);
            printf("Producent: Dodano -> %s\n", data->buffer[data->in]);

            data->in = (data->in + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&data->sem_mutex);
            sem_post(&data->sem_full);
            sleep_random();
        }
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Konsument
        for (int i = 0; i < TOTAL_MESSAGES; i++) {
            sem_wait(&data->sem_full);
            pthread_mutex_lock(&data->sem_mutex);

            printf("Konsument: Odebrano -> %s\n", data->buffer[data->out]);

            data->out = (data->out + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&data->sem_mutex);
            sem_post(&data->sem_empty);
            sleep_random();
        }
        exit(0);
    }

    // Czekamy na zakończenie obu procesów
    wait(NULL);
    wait(NULL);

    // Sprzątanie
    sem_destroy(&data->sem_empty);
    sem_destroy(&data->sem_full);
    pthread_mutex_destroy(&data->sem_mutex);
    munmap(data, SHM_SIZE);
    shm_unlink(SHM_NAME);
    close(fd);

    return 0;
}
