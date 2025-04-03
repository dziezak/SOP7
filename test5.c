#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/my_shm"
#define SHM_SIZE 1024

int main(){
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(fd, SHM_SIZE);

    char* shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shm_ptr == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    strcpy(shm_ptr, "Hello from producer!");

    printf("Produced: Wiadomość zapisan do pamięci dzielonej.\n");

    munmap(shm_ptr, SHM_SIZE);
    close(fd);
    return 0;
}