#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int *shared_mem = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int not_shared_mem = 42;
    if(shared_mem == MAP_FAILED){
        perror("mmap");
        return 1;
    }

    *shared_mem = 42;

    pid_t pid = fork();
    if(pid == 0){
        *shared_mem += 10;
        not_shared_mem +=10;
        printf("Dziecko: shared: %d, not_shared: %d\n", *shared_mem,not_shared_mem);
    }else{
        wait(NULL);
        printf("Rodzic: shared: %d, not_shared: %d\n", *shared_mem, not_shared_mem);
    }

    munmap(shared_mem, sizeof(int));
    return 0;
}
