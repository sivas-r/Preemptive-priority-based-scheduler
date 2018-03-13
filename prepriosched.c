/* This program demonstrates how a pre-emptive priority based scheduler works.
	Three threads are created randomly at intervals of 3 seconds, and the running
	thread is pre-empted if the new thread is of higher or equal priority.
*/

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>
#include<stdbool.h>

#define NUM_OF_THREADS 3
#define THREAD1_CYCLE_TIME 10
#define THREAD2_CYCLE_TIME 5
#define THREAD3_CYCLE_TIME 2
#define SCHED_CYCLE_TIME 3.0
#define NUM_OF_RESOURCES 1
#define PROCESS_SHARING 0

/* Common resource to be used by all threads. */
char resource[18];
int rindex = 0;

/* Thread ids */
pthread_t tid[NUM_OF_THREADS];

/* Mutex to lock resource. */
pthread_mutex_t lock;

/* Thread conditional variables. */
pthread_cond_t cond[NUM_OF_THREADS];

/* Structure that contains info about each thread. */
typedef struct
{
	bool wait;
	bool completed;
	int priority;
	
	void*(*threadfun)(void*);
}thread;

thread thr[NUM_OF_THREADS];

/* Function pertaining to thread 1 of burst time 10 secs (LOW priority). */
void* fun1(void* arg)
{
	static int i = 0;
	
	/* Mutex lock */
	pthread_mutex_lock(&lock);
		
	for(; i < THREAD1_CYCLE_TIME; ++i)
	{
		/* Thread waits without consuming CPU cycles unless it's conditionally invoked again. */

		while(thr[0].wait)
			pthread_cond_wait(&cond[0], &lock);
		
		resource[rindex++] = 'a';
		resource[rindex] = '\0';
		
		printf("Thread 1 (LOW priority) is using resource %s\n", resource);
		sleep(1);
	}
	thr[0].completed = true;
	pthread_mutex_unlock(&lock);
	return NULL;
}

/* Function pertaining to thread 2 of burst time 5 secs (HIGH priority). */
void* fun2(void* arg)
{
	static int i = 0;

	pthread_mutex_lock(&lock);
		
	for(; i < THREAD2_CYCLE_TIME; ++i)
	{
		
		while(thr[1].wait)
			pthread_cond_wait(&cond[1], &lock);
			
		resource[rindex++] = 'b';
		resource[rindex] = '\0';

		printf("Thread 2 (HIGH priority) is using resource %s\n", resource);
		sleep(1);
	}
	thr[1].completed = true;
	pthread_mutex_unlock(&lock);
	return NULL;
}

/* Function pertaining to thread 3 of burst time 2 secs (HIGH priority). */
void* fun3(void* arg)
{
	static int i = 0;
	
	pthread_mutex_lock(&lock);
		
	for(; i < THREAD3_CYCLE_TIME; ++i)
	{
		
		while(thr[2].wait)
			pthread_cond_wait(&cond[2], &lock);
			
		resource[rindex++] = 'c';
		resource[rindex] = '\0';
			
		printf("Thread 3 (HIGH priority) is using resource %s\n", resource);
		sleep(1);
	}
	thr[2].completed = true;
	pthread_mutex_unlock(&lock);
	return NULL;
}

/* Structure and functions to create and maintain thread queue based on priority */

struct threadq
{
	int tnum;
	struct threadq* next;
};

typedef struct threadq threadq;
threadq stack;
threadq* top = NULL;

/* Pushes to threadq based on priority. */

void pushThread(int num)
{
	if(!top)
	{
		top = malloc(sizeof(threadq));
		top -> tnum = num;
		top -> next = NULL;
	}
	
	else
	{
		threadq* temp;
		threadq* temp2;
		
		if(thr[num].priority >= thr[top -> tnum].priority)
		{
			temp = top;
			top = malloc(sizeof(threadq));
			top -> tnum = num;
			top -> next = temp;
		}
		
		else
		{
			temp = top;
			while((temp -> next != NULL) && (thr[temp -> next -> tnum].priority > thr[num].priority))
				temp = temp -> next;
		
			temp2 = temp -> next;
		
			temp -> next = malloc(sizeof(threadq));
			temp -> next -> tnum = num;
			temp -> next -> next = temp2;
		}
	}
	return;
}

/* Pops from threadq if thread is done. */ 

void popThread(void)
{
	if(top)
	{
		threadq* temp = top -> next;
		free(top);
		top = temp;
	}
	
	return;
}

/* Function to check whether all threads are completed. */

bool isAllCompleted(void)
{
	int i;
	for(i = 0; i < NUM_OF_THREADS; ++i)
	{
		if(!thr[i].completed)
			return false;
	}
	return true;
}

int max = NUM_OF_THREADS - 1;

/* Function to generate random threads. */

int RandThreadGen(void)
{
	static int thrnum[] = {0, 1, 2};
	int i, num;
	clock_t t;
	int calledthread;
	
	if(max+1)
	{
		srand(time(0));	
    	num = rand() % (max + 1);
		
		calledthread = thrnum[num];	
		printf("Thread %d is called\n", thrnum[num] + 1);
    	pthread_create(&tid[thrnum[num]], NULL, thr[thrnum[num]].threadfun, NULL);
    
    	for(i = num; i < max ; ++i)
    	{
    		thrnum[i] = thrnum[i + 1];
		}
		max--;
	}
	
   	return calledthread;  
}

/* Scheduler that calls three threads in random sequence at intervals of 3 secs and schedules them on pre-emptive priority basis. */

void MyScheduler(void)
{
	clock_t t;
	int calledthread;
	
	while(1)
	{	
		/* Call random thread if all threads haven't been called and push into thread queue based on priority. */
		if(max + 1)
		{
			calledthread = RandThreadGen();
			pushThread(calledthread);
		}
		
		/* Run thread of top priority. */
		thr[top -> tnum].wait = false;
		pthread_cond_signal(&cond[top -> tnum]);
		
		if(thr[top -> tnum].completed)
		{
			popThread();
			if(top)
			{
				thr[top -> tnum].wait = false;
				pthread_cond_signal(&cond[top -> tnum]);					
			}
		}
		
		/* Wait for 3 secs or until running thread is over. */
		if(max+1)
		{
			t = clock();
			while(((double)(clock() - t)/CLOCKS_PER_SEC) < SCHED_CYCLE_TIME)
			{
				if(thr[top -> tnum].completed)
					break;
			}
			thr[top -> tnum].wait = true;	
		}
			
		if(isAllCompleted())
			return;		
	}
}

void Init(void)
{
	int i;
		
	/* Thread properties such as priority and state are initialized. */
	
	for(i = 0; i < 3; ++i)
	{
		thr[i].wait = true;
		thr[i].completed = false;
		switch(i)
		{
			case 0:
				thr[i].threadfun = fun1;
				thr[i].priority = 0;
			break;
			case 1:
				thr[i].threadfun = fun2;
				thr[i].priority = 1;
			break;
			case 2:
				thr[i].threadfun = fun3;
				thr[i].priority = 1;
			break;
		}
	}
	
	printf("Priority of thread 1 is MIN\n");
	printf("Priority of thread 2 is MAX\n");
	printf("Priority of thread 3 is MAX\n");
	
	return;
}

int main()
{
	int i;
	
	/* Thread properties are initialized. */
	Init();
	
	/* Mutex lock is initialized. */
	pthread_mutex_init(&lock, NULL);	
	
	/* Thread conditional variables are initialized.*/
	for(i = 0; i < NUM_OF_THREADS; ++i)
		pthread_cond_init(&cond[i], NULL);
		
	sleep(1);
	
	/* Scheduler is invoked. */
	MyScheduler();
	
	/* Condition variables and mutex are destroyed. */
	for(i = 0; i < NUM_OF_THREADS; ++i)
		pthread_cond_destroy(&cond[i]);	
	
	pthread_mutex_destroy(&lock);	
	
	return 0;
}
