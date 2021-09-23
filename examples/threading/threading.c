#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    // Sleep for specified microseconds
    usleep(thread_func_args->wait_to_obtain_ms);
    
    // Check if lock is acquired
    if (pthread_mutex_lock(thread_func_args->thread_mutex) != 0){

        perror("\nERROR THREAD LOCKING");
        ERROR_LOG("\nERROR THREAD LOCKING");

        thread_func_args->thread_complete_success = false;         // Set thread completion status to false

        return thread_param;

    }
    
    // Sleep for specified microseconds
    usleep(thread_func_args->wait_to_release_ms);
    
    // Check if lock is release
    if (pthread_mutex_unlock(thread_func_args->thread_mutex) != 0){

        perror("\nERROR THREAD UNLOCKING");
        ERROR_LOG("\nERROR THREAD UNLOCKING");

        thread_func_args->thread_complete_success = false;         // Set thread completion status to false


        return thread_param;

    }

    thread_func_args->thread_complete_success = true;              // Set thread completion status to true

    DEBUG_LOG("\nTHREAD COMPLETED SUCCESSFULLY");

    return thread_param;

}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     * 
     */

    pthread_t new_thread;               

    int create_check;                 // Variable to check pthread_create() status
     
    // Allocate memory to thread data structure
    struct thread_data *data = (struct thread_data*)malloc(sizeof(struct thread_data));
    
    // Assign mutex lock for thread to lock and unlock
    data->thread_mutex=mutex;
   
    // Assign wait time parameters for thread
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    
    // Create Thread
    create_check = pthread_create(&new_thread,NULL,threadfunc,(void*)data);
    
    // Check thread creation
    if (create_check == 0){

        DEBUG_LOG("\nTHREAD CREATED SUCCESSFULLY");

        *thread = new_thread;

        return true;
    
    }

    // Else Log Error in thread creation 
    perror("\nERROR THREAD CREATION");
    ERROR_LOG("\nERROR THREAD CREATION");

    return false;
    
}

