#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
// Max size of the shared buffer
#define BUFFSIZE 500
// Number of worker threads to create
#define NUM_THREADS 10
// Random numbers produced per producer thread
#define NUMS_PER_THREAD 1000
// Time quantum of roundrobin (in ms)
#define delta 1000
using namespace std;
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
vector<int> status(NUM_THREADS);    // the status vector
/*
    status = 0 --> active consumer
    status = 1 --> active producer
    status = 2 --> terminated/dead producer
    status = 3 --> terminated/dead consumer
*/
int buffer[BUFFSIZE];               // The buffer to store the produced items
int buffer_count = 0;               // the number of items currently in the buffer
int in = 0;                         // the current first for producing
int out = 0;                        // the current index for consumption
pthread_mutex_t sig_mutex;          // the variable for mutex lock
pthread_mutexattr_t ma;             // the attribute for mutex lock
int current_thread = -1;            // the current executing thread

//The signal handler function to stop or play threads
void signal_handler(int signo)
{
    // If signal SIGUSR1 is given, then pause the current thread until next signal
    if(signo == SIGUSR1){
        pause();
    }
}

// The runner function for worker threads. This function uses status to find\
if the thread is a producer or a consumer
int count_ones(vector<int>&v)
{
      int res=0;
      for(auto &x:v)
        res+=x==1;
      return res;
}
int count_zeroes(vector<int>&v)
{
       int res=0;
       for(auto &x:v)
        res+=x==0;
       return res;
}
void * runner (void * param)
{


    int id = *(int*)param;              // get the id
    int nums_left = NUMS_PER_THREAD;    // If the thread is a producer then the \
    then the number of items it has to consume.
    
    // Pause to wait for the first signal
    pause();

    // If the thread is a producer 
    if(status[id]==1){
        // Keep continuing
        for(;;)
        {
            for(;nums_left && buffer_count!=BUFFSIZE;)
            {
                pthread_mutex_lock(&sig_mutex);         //start critical section
                int new_num = rng()%1000+1;            // generate a new number
                buffer[in] = new_num;                   // add the number to the buffer
                in = (in+1)%BUFFSIZE;                   // increment the input buffer
                buffer_count++;                         // Increment the buffer count
                nums_left--;                            // Decrease the number left for the particular\
                                                            thread to follow
                pthread_mutex_unlock(&sig_mutex);       // end critical section
            }
            if(nums_left==0){                           // If while loop breaks
                pthread_mutex_lock(&sig_mutex);         // lock for CS
                status[id]=2;                           // set the status to producer complete
                pthread_mutex_unlock(&sig_mutex);       // unlock mutex
                pthread_exit(0);                        // exit the thread
            }
        }
    }
    // If the thread is a consumer
    else if(status[id]==0){
        for(;;)// Keep executing till the thread exits
        {
            for(;buffer_count>0;)
            {                  // While there is still left to consume
                pthread_mutex_lock(&sig_mutex);     // Mutex lock
                int new_num = buffer[out];          // get the number from the buffer
                out = (out+1)%BUFFSIZE;             // Increment the out index
                buffer_count--;                     // Decrement the buffer count
                pthread_mutex_unlock(&sig_mutex);   // Unlock the buffer
            }
            pthread_mutex_lock(&sig_mutex);         // lock the mutex for end cond check
            // if everything has been completed
            if(count_ones(status) == 0 && buffer_count <= 0)
            {
                status[id] = 3;
                pthread_mutex_unlock(&sig_mutex);   // unlock the mutex
                pthread_exit(0);                    // exit the thread
            }
            pthread_mutex_unlock(&sig_mutex);       // unlock the mutex.
        }
    }
}

//The reporter thread
void * reporter(void* param)
{
    // Store the previous thread
    int prev_thread = -1;

    // wait for at least one thread to start
    for(;current_thread==-1;);
    for(;;)
    {
        pthread_mutex_lock(&sig_mutex);
        if(current_thread == -2)    // If execution is over
        {
            printf("Consumer Thread ");
            printf("%d",prev_thread);
            printf(" has terminated");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");
            pthread_exit(0);
        }
        if( current_thread == prev_thread){ // If no change in the thread
            pthread_mutex_unlock(&sig_mutex);
            continue;
        }
        // If the execution of first thread is starting
        if(prev_thread == -1){
            printf("Execution of ");
            printf("%d",current_thread);
            printf(" has started");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");

        }
        else
        // If scheduler thread is still active, then print the context switch message
        if(current_thread != -2){
            printf("The execution has changed from ");
            printf("%d",prev_thread);
            printf(" to ");
            printf("%d",current_thread);
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");            
        }
        // If the context switch happened and old thread has terminated
        if((status[prev_thread] == 2 || status[prev_thread] == 3) && current_thread != -2){
            if(status[prev_thread]==2)
                printf("Producer ");
            else
                printf("Consumer"); 
            printf("thread ");
            printf("%d",prev_thread);
            printf(" has terminated");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");
        }
        // If scheduler is still active, then update old thread's value
        if(current_thread != -2)
            prev_thread = current_thread;
        pthread_mutex_unlock(&sig_mutex);

    }

}

// The scheduler thread
void* scheduler(void* param)
{
    // List of worker thread's tid values
    pthread_t* mythreads = (pthread_t*)param;
    queue<int> scheduler_q;         // The FIFO queue to schedule the threads
    int i=0;
    while(i<NUM_THREADS)
    {
           scheduler_q.push(i);
           i++;
    }

    for(;;)
    {
        pthread_mutex_lock(&sig_mutex); //Lock the mutex at the start of the scheduler

        //Count the number of active threads        
        int n=count_ones(status);
        int m=count_zeroes(status);
        // If the exit condition is true
        if(n == 0 && buffer_count <= 0 && m==0){
            current_thread = -2;
            pthread_mutex_unlock(&sig_mutex);// unlock mutex before exiting
            pthread_exit(0);
        }
        
        // get the next thread from the queue
        int curr = scheduler_q.front();
        scheduler_q.pop();

        // If it is an active thread
        if(status[curr]== 0 || status[curr]== 1)
            pthread_kill(mythreads[curr],SIGUSR2);
        current_thread = curr;
        pthread_mutex_unlock(&sig_mutex);// unlock the mutex and let the worker does its thing

        usleep(delta);//Wait for the worker to do stuff
        
        // Lock for stopping the thread     
        pthread_mutex_lock(&sig_mutex);


        // Send the next signal for the thread to stop - only if it has not yet terminated
        if(status[curr]== 0 || status[curr]== 1)
            pthread_kill(mythreads[curr],SIGUSR1);
        
        // If the thread is not complete yet push it back
        if(status[curr] != 2 && status[curr] != 3){
            scheduler_q.push(curr);
        }

        // unlock the mutex for the next one
        pthread_mutex_unlock(&sig_mutex);
    }
}
void * test_without_locks(void* param)
{
    int prev_thread = -1;
    for(;;)
    {
        if(current_thread == -2)    // If execution is over
        {
            printf("Consumer Thread ");
            printf("%d",prev_thread);
            printf(" has terminated");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");
            pthread_exit(0);
        }
        if( current_thread == prev_thread){ // If no change in the thread
            continue;
        }
        if(prev_thread == -1)
        {
            printf("Execution of ");
            printf("%d",current_thread);
            printf(" has started");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");

        }
        else
        if(current_thread != -2)
        {
            printf("The execution has changed from ");
            printf("%d",prev_thread);
            printf(" to ");
            printf("current_thread");
            printf("\n");
            printf("Current buffer size = ");
            printf("%d",buffer_count);
            printf("\n");            
        }
    }
}
// The main thread
int main()
{
    pthread_mutexattr_init(&ma);// Initialise the mutex attributes to default values
    // The mutex has to be recursive
    pthread_mutexattr_settype(&ma,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sig_mutex,&ma);     // Initialse the mutex lock with the attributes

    pthread_t mythreads[NUM_THREADS];// declare the thread variables
    pthread_t sched_thread, reporter_thread;// declare the reporter and the scheduler
    pthread_attr_t attr;// Thread attributes
    pthread_attr_init(&attr);// Initialise pthread attributes to default values
    srand(time(NULL));// Set the random number seedd
    
    int id,i;// Variable to pass id to the function

    signal(SIGUSR1,signal_handler);// install the signal handlers
    signal(SIGUSR2,signal_handler);
    i=0;
    while(i<NUM_THREADS)
    {// Declare the threads
        int* id = new int;// Set the id
        *id = i;
        if(rng()%2==0){// Set if producer or consumer
            status[i] = 0;
            printf("Consumer thread ");
            printf("%d",i);
            printf(" is created");
            printf("\n");
        }
        else{ 
            status[i] = 1;
            printf("Producer Thread ");
            printf("%d",i);
            printf(" is created");
            printf("\n");
        }
        pthread_create(&mythreads[i],NULL,runner,(void *)id);
        i++;
    }
    // create the scheduler thread
    pthread_create(&sched_thread,NULL,scheduler,mythreads);
    // create the reporter thread
    pthread_create(&reporter_thread,NULL,reporter,NULL);

    // wait for all threads to join
    i=0;
    while(i<NUM_THREADS)
    {
        pthread_join(mythreads[i],NULL);
        i++;
    }
    // wait for scheduler to join
    pthread_join(sched_thread,NULL);
    printf("Scheduler thread has terminated");
    printf("\n");
    // wait for reporter to join    
    pthread_join(reporter_thread,NULL);
    printf("Reported thread has terminated");
    printf("\n");
    printf("Everything done");
    printf("\n");
    // destroy the mutex at the end
    pthread_mutex_destroy(&sig_mutex);
    return 0;
}