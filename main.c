
/****************************************
 * File:   main.c                       *
 * Author: Arttu Lehtovaara             *
 *                                      *
 * Created on March 24, 2019, 12:29 PM  *
 ****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// reader and writer threads will increment these value
int sum_of_succesful_reads = 0;
int sum_of_failed_reads = 0;
int sum_of_succesful_writes = 0;
int sum_of_failed_writes = 0;

char buffer[10] = "123456789"; // max is 9 chars cause trailing '\0' should fit
int reader_thread_count = 0;
int writer_thread_count = 0;
int read_loops = 0;
int write_loops = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void write_random_data(char* buf, int len)
{
    int i;
    for (i = 0; i < len - 1; i++) // len-1 since null has to fit after real data
    {
        buf[i] = rand()% 25 + 65; // A..Z
    }
    buf[i] = '\0'; // null char after last "real" data char 
}

// ----------------------------------------------------------------------------------------
// conditional compilation flag MUTEX_IMPLEMENTATION
// if commented out --> reader_writer_version
#define MUTEX_IMPLEMENTATION
// declare global data buffer and other global variables if needed


#ifdef MUTEX_IMPLEMENTATION

// declare global mutexes
pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;

int read_count = 0;
// thread functions
void* reader(void* args)
{
    int i;
    int fail = 0;
    for (i = 0; i < read_loops; i++) 
    {
        // entering critical section
        fail = 0;
        pthread_mutex_lock(&mutex);
        read_count++;
        if (read_count == 1) // if the first reader..
        {
            if (pthread_mutex_trylock(&rw_mutex)) //already locked by writer?
            {
                fail = 1;
                sum_of_failed_reads++;
            }
        }
        pthread_mutex_unlock(&mutex);
        if (fail == 0)
        {
            pthread_mutex_lock(&mutex);
            sum_of_succesful_reads++;
            pthread_mutex_unlock(&mutex);
            // now we are in the critical section ready to read buffer data
            printf("read: %s\n", buffer);
        }
        // exit from critical section
        pthread_mutex_lock(&mutex);
        read_count--;
        if (read_count == 0 && fail == 0)
        {
            pthread_mutex_unlock(&rw_mutex);
        }
        pthread_mutex_unlock(&mutex);
        //pthread_yield(); yield() voluntarily cpu to another thread
        sleep(rand()%3);
    }
    return NULL;
}
void* writer(void* args)
{
    int i;
    int fail = 0;
    for (i = 0; i < write_loops; i++) 
    {
        fail = 0;
        // enter critical section..
        if (pthread_mutex_trylock(&rw_mutex))
        {
            fail = 1;
            pthread_mutex_lock(&mutex);
            sum_of_failed_writes++;
            pthread_mutex_unlock(&mutex);
        }
        // writing, if not failed..
        if (fail == 0)
        {
            pthread_mutex_lock(&mutex);
            sum_of_succesful_writes++;
            pthread_mutex_unlock(&mutex);
            write_random_data(buffer, 10);
            printf("Write: %s\n", buffer);
        }
        // exit critical section
        if (fail == 0)
        {
            pthread_mutex_unlock(&rw_mutex);
        }
        //pthread_yield();
        sleep(rand()%3);
    }
    return NULL;
}

#else

// declare global reader_writer_locks
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
// thread functions
void* reader(void* args)
{
    int i;
    int fail = 0;
    for (i = 0; i < read_loops; i++)
    {
        if (pthread_rwlock_tryrdlock(&rwlock)) // if fails
        {
            fail = 1;
            pthread_mutex_lock(&mutex);
            sum_of_failed_reads++; // if fails, increment read counter
            pthread_mutex_unlock(&mutex);
        }
        if (!fail)
        {
            pthread_mutex_lock(&mutex);
            sum_of_succesful_reads++;
            pthread_mutex_unlock(&mutex);
            printf("read: %s\n", buffer);
            pthread_rwlock_unlock(&rwlock);
        }
        //pthread_yield();
        sleep(rand()%3);
    }
    return NULL;
}
void* writer(void* args)
{

    int i;
    int fail = 0;
    for (i = 0; i < write_loops; i++) 
    {
        //fail = 0;
        // enter critical section..
        if (pthread_rwlock_trywrlock(&rwlock))
        {
            fail = 1;
            pthread_mutex_lock(&mutex);
            sum_of_failed_writes++;
            pthread_mutex_unlock(&mutex);
        }
        // writing, if not failed..
        if (!fail)//fail == 0)
        {
            pthread_mutex_lock(&mutex);
            sum_of_succesful_writes++;
            pthread_mutex_unlock(&mutex);
            write_random_data(buffer, 10);
            printf("Write: %s\n", buffer);
            pthread_rwlock_unlock(&rwlock);
        }
        // exit critical section
        //pthread_yield();
        sleep(rand()%3);
    }
    return NULL;
}

#endif

int main(int argc, char* argv[])
{
    
    //first check command line arguments
    // 1) should be 5 (progname + 4 actual)
    if (argc != 5)
    {
        printf("usage <prog> <wr_thread_count> <rd_thread_count> <wr_loops> <rd_loops>\n");
        exit(-1);
    }
    //2) all argv[1]..argv[4] ints?
    writer_thread_count = atoi(argv[1]); // can use also scan (or atoi)
    reader_thread_count = atoi(argv[2]);
    write_loops = atoi(argv[3]);
    read_loops = atoi(argv[4]);
    if (writer_thread_count <= 0 || reader_thread_count <= 0 || write_loops <= 0 || read_loops <= 0)
    {
        printf("illegal arguments!\n");
        exit(-1);
    }
    // declare and initialize threads and threads attrs
    pthread_attr_t pat; // this attr can be used in every threads
    pthread_t writers[writer_thread_count];
    pthread_t readers[reader_thread_count];
    pthread_attr_init(&pat);

    // create writer and reader threads
    int i;
    for (i = 0; i < writer_thread_count; i++)
    {
        pthread_create(&writers[i], &pat, writer, NULL);
    }
    for (i = 0; i < reader_thread_count; i++) 
    {
        pthread_create(&readers[i], &pat, reader, NULL);
    }
    
    // join them.. one by one.. use for loop
    for (i = 0; i < writer_thread_count; i++) 
    {
        pthread_join(writers[i], NULL);
    }
    for (i = 0; i < reader_thread_count; i++) 
    {
        pthread_join(readers[i], NULL);
    }
    
    printf("\nWrite: Threads count: %d, Loops count: %d, Summary: %d, Successful write: %d, Failed write: %d\n",
            writer_thread_count, write_loops, sum_of_succesful_writes+sum_of_failed_writes,
            sum_of_succesful_writes, sum_of_failed_writes);
    printf("Read: Threads count: %d, Loops count: %d, Summary %d, Successful read: %d, Failed read: %d\n",
            reader_thread_count, read_loops, sum_of_succesful_reads+sum_of_failed_reads,
            sum_of_succesful_reads, sum_of_failed_reads);
    

    // print successful and failed reads and writes
    // also print - as a comparison check - how many read and write 
    // totals (failed + successful) should have been
    
    return 0;
}
