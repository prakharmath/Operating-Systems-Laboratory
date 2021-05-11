#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<set>
#include<algorithm>
#include<chrono>
#include<unistd.h>
#include<assert.h>
#include<sys/types.h>
#include<fcntl.h>  
#include<sys/ipc.h>
#include<sys/shm.h>
#include<semaphore.h>
using namespace std;
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
struct Job
{
	int process_id;
	int producer_number;
	int prior;	// lower number means higher priority
	int comp_time;
	int job_id;
	bool operator<(const Job &rhs) const
	{
		return prior < rhs.prior;
	}
	void show()
	{
		printf("Details of the job!");
		printf("\n");
		printf("Process ID: ");
		printf("%d",process_id);
		printf("\n");
		printf("Producer_Number: ");
		printf("%d",producer_number);
		printf("\n");
		printf("Priority: ");
		printf("%d",prior);
		printf("\n");
		printf("Computation_time: ");
		printf("%d",comp_time);
		printf("\n");
		printf("JOB ID: ");
		printf("%d",job_id);
		printf("\n");
	}
};
int NP, NC, TotalJobs;
const int JMAX=15;
struct job_queue
{	
	Job jq[JMAX];
	int jsize;
	job_queue():jsize(0){}
	bool is_empty()
	{
		return (jsize == 0);
	}
    bool is_full()
    {
		return (jsize == JMAX);
	}
	void insert_job(Job j)
	{
		assert((this->is_full()) ==  false);
		jq[jsize++] = j;
		sort(jq, jq+jsize); // can be optimised
	}
	Job top()
	{ 
	 // gets the highest priority job, and removes it from the job_queue
		assert((this->is_empty()) == false);
		Job ret = jq[0];
		int i=0;
		while(i<jsize-1)
		{
			  jq[i]=jq[i+1];
			  i++;
		}
		jsize--;
		return ret;
	}
};
void show_job_queue(job_queue x)
{
       printf("The job queue looks like");
       printf("\n");
       for(int i=0;i<x.jsize;i++)
       {
       	     printf("%d",x.jq[i].process_id);
       	     printf("->");
       }
       printf("\n");
       return;
}
Job create_job(int prod_num, int proc_id)
{
	Job j;
	j.process_id = proc_id;
	j.producer_number = prod_num;
	j.prior = 1 + (rng()%10);
	j.comp_time = 1 + (rng()%4);
	j.job_id = 1 + (rng()%100000);
	return j;
}
struct shared_data
{
	int created_jobs;
	int finished_jobs;
	int total_jobs;
	job_queue jq;
	sem_t sem_consumer;
	sem_t sem_producer;
	void init(){
		created_jobs = 0;
		finished_jobs = 0;
		total_jobs = -1;
		sem_init(&sem_consumer, 1, 1);
		sem_init(&sem_producer, 1, 1);
	}
};
void show_shared_data(shared_data x)
{
	  printf("Created Jobs:");
	  printf("%d",x.created_jobs);
	  printf("\n");
	  printf("Finished Jobs: ");
	  printf("%d",x.finished_jobs);
	  printf("\n");
	  printf("Total Jobs: ");
	  printf("%d",x.total_jobs);
	  printf("\n");
	  printf("Job queue looks like");
	  printf("\n");
	  for(int i=0;i<x.jq.jsize;i++)
	  {
             printf("%d",x.jq.jq[i].process_id);
             printf("->");
	  }
	  printf("\n");
	  return;
}
void show_job(Job J)
{
      	printf("Details of the job!");
		printf("\n");
		printf("Process ID: ");
		printf("%d",J.process_id);
		printf("\n");
		printf("Producer_Number: ");
		printf("%d",J.producer_number);
		printf("\n");
		printf("Priority: ");
		printf("%d",J.prior);
		printf("\n");
		printf("Computation_time: ");
		printf("%d",J.comp_time);
		printf("\n");
		printf("JOB ID: ");
		printf("%d",J.job_id);
		printf("\n");	 
}
signed main()
{
	time_t tstart, tend;
	printf("Please enter the number of producer processes: ");
	scanf("%d",&NP);
	printf("\n");
	printf("Please enter the number of consumer processes: ");
	scanf("%d",&NC);
	printf("\n");
    printf("Enter number of jobs: ");
	scanf("%d",&TotalJobs);
	printf("\n");
	time(&tstart);
	key_t key = ftok("/dev/random", 'b');
	int shmid = shmget(key, 2048, 0666 | IPC_CREAT);
	shared_data *sd = (shared_data *) shmat(shmid, (void *)0, 0);
	sd->init();
	sd->total_jobs = TotalJobs;
	int prod_num=1;
	int is_producer = 0; // 1, if its a producer process 
	while(prod_num<=NP)
	{
		int pid = fork();
		if (!pid)
		{
			is_producer = 1;
			if (is_producer){
				for(;(sd->total_jobs==-1););
				 // spin
				for(;(sd->created_jobs) < (sd->total_jobs);)
				{
					if (((sd->jq).is_full()))
						continue;
					// else, take the producer lock
					sem_wait(&(sd->sem_producer));
					if ((sd->created_jobs) < (sd->total_jobs) && !((sd->jq).is_full()))
					{
						// sleep for a random time between 0 and 3 seconds
						int rtime = rng() % 4;
						sleep(rtime);
						int job_id = (sd->created_jobs) + 1;
						(sd->created_jobs)++;
						Job j_created = create_job(prod_num, job_id);
						// j_created.show();
						(sd->jq).insert_job(j_created);
						printf("Producer ");
						printf("%d",prod_num);
						printf(" created job ");
						printf("%d",job_id);
						printf("\n");
						j_created.show();
					}
					sem_post(&(sd->sem_producer));
				}
				shmdt(sd);
				return 0;
			}
			break; //  this is essential or otherwise child process will start spawning
		}
		else
		{
			printf("[producer number = ");
			printf("%d",prod_num);
			printf(" , pid= ");
			printf("%d",pid);
			printf(" ]");
			printf("\n"); 
		}
		prod_num++;
	} 
	int con_num=1;
	int is_consumer = 0; // 1, if its a consumer process 
	while(con_num<=NC)
	{
		int pid = fork();
		if (!pid)
		{
			is_consumer = 1;
			if (is_consumer){
				 // spin
				for(;(sd->total_jobs)==-1;);
			    for(;(sd->finished_jobs) < (sd->total_jobs);)
				{
					if (((sd->jq).is_empty()))
						continue;
					// else, take the consumer lock
					sem_wait(&(sd->sem_consumer));
					if ((sd->finished_jobs) < (sd->total_jobs) and !((sd->jq).is_empty()))
					{
						// sleep for a random time between 0 and 3 seconds
						printf("Consumer process ");
						printf("%d",con_num);
						printf(" executing");
						printf("\n");
						int rtime = rng() % 4;
						sleep(rtime);
						(sd->finished_jobs)++;
						Job j = (sd->jq).top();
						j.show();
						sleep(j.comp_time);// job sleep
					}
					sem_post(&(sd->sem_consumer));
				}
				shmdt(sd);
				return 0;
			}
			break; //  this is essential or otherwise child process will start spawning
		}
		else
		{
			printf("[consumer number = ");
			printf("%d",con_num);
			printf(" , pid = ");
			printf("%d",pid);
			printf(" ]");
			printf("\n");
		}
		con_num++;
	} 
   // spin lock on main proc
	for(;((sd->created_jobs) != TotalJobs || (sd->finished_jobs) != TotalJobs););
	time(&tend);
	double time_taken = double(tend - tstart);
	printf("Time taken to execute: ");
	printf("%.7lf",time_taken);
	printf(" s");
	printf("\n");
	shmdt(sd);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}