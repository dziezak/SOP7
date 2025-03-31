#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(){
    int fd = open("test.txt", O_RDWR);
    if(fd == -1){
        perror("open");
        return 1;
    }

    struct stat sb;
    if(fstat(fd, &sb) == -1){
        perror("fstat");
        return 1;
    }

    char *mapped = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(mapped == MAP_FAILED){
        perror("mmap");
        return 1;
    }

    mapped[0] = 'X';

    msync(mapped, sb.st_size, MS_SYNC);
    munmap(mapped, sb.st_size);
    close(fd);

    return 0;
}