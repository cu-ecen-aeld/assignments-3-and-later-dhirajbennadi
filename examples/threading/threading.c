#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MILLS_TO_MICROS     1000
#define SUCCESS              0

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    usleep(thread_func_args->wait_to_obtain_ms * MILLS_TO_MICROS);

    if(!(pthread_mutex_lock(thread_func_args->mutexParameter)))
    {
        DEBUG_LOG("Mutex Lock Successful");
    }
    else
    {
        ERROR_LOG("Mutex Locking Failed\n");
    }

    usleep(thread_func_args->wait_to_release_ms * MILLS_TO_MICROS);

    if(!pthread_mutex_unlock(thread_func_args->mutexParameter))
    {
        DEBUG_LOG("Mutex UnLock Successful");
    }
    else
    {
        ERROR_LOG("Mutex UnLock Failed\n");
    }

    thread_func_args->threadId = pthread_self();
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int retVal = -1;
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    /*Allocating memory for thread data*/

    printf("Dhiraj Bennadi*****************************\n");

    struct thread_data *thread_data_ptr = (struct thread_data *)malloc(sizeof(struct thread_data));
    if(thread_data_ptr == NULL)
    {
        ERROR_LOG("Dynamic Allocation Failed\n");
        return false;
    }


    thread_data_ptr->mutexParameter = mutex;
    thread_data_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_ptr->wait_to_release_ms = wait_to_release_ms;
    thread_data_ptr->thread_complete_success = false;

    

    // /*Creation and Initialization of Mutex*/
    // pthread_mutex_t m;

    // if(pthread_mutex_init(&m, NULL) != 0)
    // {
    //     printf("Mutex Initialization Failed\n");
    //     return false;

    // }

    /*Thread Creation*/
    //pthread_t userThread;

    retVal = pthread_create(thread, NULL, threadfunc, (void*)thread_data_ptr);

    if(retVal == SUCCESS)
    {
        DEBUG_LOG("Thread ID = %ld************\n", *thread);
        thread_data_ptr->threadId = *thread;

    }
    else
    {
        ERROR_LOG("Thread Creation Failed\n");
    }

    if(thread_data_ptr != NULL)
    {
        free(thread_data_ptr);
    }

    //pthread_join(*thread , NULL);

    if(retVal == SUCCESS)
    {
        return true;
    }

    return false;
}

