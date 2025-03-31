#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int *shared_mem = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(shared_mem == MAP_FAILED){
        perror("mmap");
        return 1;
    }

    *shared_mem = 42;

    pid_t pid = fork();
    if(pid == 0){
        printf("Dziecko: %d\n", *shared_mem);
    }else{
        *shared_mem += 10;
        wait(NULL);
        printf("Rodzic: %d\n", *shared_mem);
    }

    munmap(shared_mem, sizeof(int));
    return 0;
}
