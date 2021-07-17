#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
 
using namespace std;
 
#define NUM_THREADS  5

unsigned long share_value = 0;
pthread_mutex_t mutex;

void *wait(void *t)
{
    struct timeval begin, end;
    int tid, print_index;
    gettimeofday(&begin, 0);
    tid = *((int*)t);

    while(tid > 0)
    {
        tid --;
        
        pthread_mutex_lock(&mutex);
        print_index = share_value;
        pthread_mutex_unlock(&mutex);

        cout << "Sleeping in thread num: " << print_index << endl;
        usleep(1000*5);
        
        gettimeofday(&end, 0);
        float sec_loop = ((end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)*1e-6);
        printf("time: %.02f ms; rate:%.02f \n", sec_loop*1000.0, 1/sec_loop);
        gettimeofday(&begin, 0);
    }
    pthread_exit(NULL);
}
 
int main(void)
{
   int rc;
   int i;
   pthread_t threads[NUM_THREADS];
   pthread_attr_t attr;
   void *status;
 
   // 初始化并设置线程为可连接的（joinable）
//    pthread_attr_init(&attr);
//    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&mutex, NULL);

    i = 100;
    rc = pthread_create(&threads[0], NULL, wait, (void *)&i);
    for (rc = 0; rc < i; rc++)
    {
        usleep(10*rc);
        cout << " ----------  Main: program adding--" << share_value++ << endl;
    }
    // rc = pthread_join(threads[0], &status);

   pthread_exit(NULL);
    pthread_mutex_destroy(&mutex);
}