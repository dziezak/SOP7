#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_PROCESSES 4
#define SHM_NAME "/char_count_shm"

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Uzycie %s <nazwa_pliku>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        perror("open in fd");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if(fstat(fd, &st) == -1){
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    size_t file_size = st.st_size;
    char *mapped_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(mapped_data == MAP_FAILED){
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    size_t shm_size = MAX_PROCESSES * 256 * sizeof(int);
    ftruncate(shm_fd, shm_size);

    int *shared_counts = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shared_counts == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memset(shared_counts, 0, shm_size); // cala tablica jest wypelniona zerami <=> counts = {0};

    for(int i=0; i<MAX_PROCESSES; i++){
        pid_t pid = fork();
        if(pid == 0){
            size_t chunk_size = file_size / MAX_PROCESSES;
            size_t start = i* chunk_size;
            //size_t end = (i+1)*chunk_size -1; // czy ok?
            size_t end = (i == MAX_PROCESSES - 1) ? file_size : (i + 1) * chunk_size;


            int *local_counts = shared_counts + i * 256;
            for(size_t j = start; j < end; j++){
                unsigned char c = mapped_data[j];
                local_counts[c]++;
            }

            munmap(mapped_data, file_size);
            munmap(shared_counts, shm_size);
            close(shm_fd);
            close(fd);
            exit(EXIT_SUCCESS);
        }
    }

    for(int i=0; i<MAX_PROCESSES; i++){
        wait(NULL);
    }
    int final_counts[256] = {0};
    for(int i=0; i<MAX_PROCESSES; i++){
        int *local_counts = shared_counts + i * 256;
        for(int j=0; j<256; j++){
            final_counts[j] += local_counts[j];
        }
    }

    printf("\n--- Podsumowanie znaków ---\n");
    for (int i = 0; i < 256; i++) {
        if (final_counts[i] > 0) {
            if (i >= 32 && i <= 126)
                printf("Znak '%c' (%d): %d razy\n", i, i, final_counts[i]);
            else
                printf("Znak niedrukowalny (%d): %d razy\n", i, final_counts[i]);
        }
    }



    munmap(mapped_data, file_size);
    munmap(shared_counts, shm_size);
    shm_unlink(SHM_NAME);
    close(fd);
    close(shm_fd);
    return 0;
}
