#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>


#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

// Values of this function are in range (0,1]
double func(double x)
{
    usleep(2000);
    return exp(-x * x);
}

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int randomize_points(int N, float a, float b)
{
    int i = 0;
    for (; i < N; ++i)
    {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            i++;
    }
    return i;
}

/**
 * This function calculates approximation of integral from counters of hit and total points.
 * @param total_randomized_points Number of total randomized points.
 * @param hit_points Number of hit points.
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return The approximation of integral
 */
double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b)
{
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

/**
 * This function locks mutex and can sometime die (it has 2% chance to die).
 * It cannot die if lock would return an error.
 * It doesn't handle any errors. It's users responsibility.
 * Use it only in STAGE 4.
 *
 * @param mtx Mutex to lock
 * @return Value returned from pthread_mutex_lock.
 */
int random_death_lock(pthread_mutex_t* mtx)
{
    int ret = pthread_mutex_lock(mtx);
    if (ret)
        return ret;

    // 2% chance to die
    if (rand() % 50)
        abort();
    return ret;
}

void usage(char* argv[])
{
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before reporting to shared memory (default: 1000)\n");
}



typedef struct{
    int process_count;
    float a, b;
    uint64_t total_points;
    uint64_t hit_points;
    int stop;
}SharedMemory;
#define SHM_NAME "/my_shm"
#define SEM_NAME "/my_sem"


SharedMemory* get_shared_ptr(SharedMemory* ptr){
    static SharedMemory* shared_ptr = NULL;
    if(ptr != NULL)
        shared_ptr = ptr;
    return shared_ptr;
}

void sigint_handler(int sig){
    SharedMemory* data = get_shared_ptr(NULL);
    if(data) data->stop = 1;
}


int main(int argc, char* argv[])
{
    float N, a, b;
    if(argc > 4){
        usage(argv);
    }
    N = 1000, a = -1, b = 1;
    if (argc >= 4) N = atoi(argv[3]);
    if (argc >= 3) b = atof(argv[2]);
    if (argc >= 2) a = atof(argv[1]);

    size_t shm_size = sizeof(SharedMemory);
    int shm_fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, 0666);
    if(shm_fd < 0) ERR("shm_open");
    
    if(ftruncate(shm_fd, shm_size) <0) ERR("fturncate");

    SharedMemory* shared_data = (SharedMemory*)mmap(NULL, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shared_data == MAP_FAILED) ERR("mmap");
    shared_data->process_count = N;
    shared_data->a = a;
    shared_data->b = b;
    shared_data->hit_points = 0;
    shared_data->total_points = 0;
    shared_data->stop = 0;

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0) ERR("sigaction");
    
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if(sem == SEM_FAILED) ERR("sem_open");

    for(int i=0; i<N; i++){
        pid_t pid = fork();
        if(pid < 0) ERR("fork");
        if(pid == 0){
            srand(getpid()^time(NULL));
            for(int batch = 0; batch < 3; batch++){
                sem_wait(sem);
                bool should_stop = (shared_data->stop == 1);
                sem_post(sem);
                if(should_stop) break;

                int hits = randomize_points(N, a, b);

                sem_wait(sem);
                shared_data->hit_points += hits;
                shared_data->total_points += N;
                printf("Proces %d | Batch %d | Trafienia: %lu / %lu\n",
                    getpid(), batch + 1, shared_data->hit_points, shared_data->total_points);
                sem_post(sem);
            }

            sleep(2);
            sem_wait(sem);
            shared_data->process_count--;
            bool is_last = (shared_data->process_count == 0);
            sem_post(sem);
            
            if(is_last){
                double result = summarize_calculations(shared_data->total_points, shared_data->hit_points, shared_data->a, shared_data->b);
                printf("\nPID[%d] FINAL RESULT: %5lf\n", getpid(), result);
            }
            sem_close(sem);
            munmap(shared_data, shm_size);
            exit(EXIT_SUCCESS);
        }
    }
    for(int i=0; i<N; i++){
        wait(NULL);
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(shared_data, shm_size);
    shm_unlink(SHM_NAME);

    return EXIT_SUCCESS;
}