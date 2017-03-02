#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "randomSleep.h"
#include <locale.h>
#include <pthread.h>
#include <string.h> //for memcpy

//for sleep function
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

//pthread stuff
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //for locking
//pthread_cond_t nonEmpty  = PTHREAD_COND_INITIALIZER; //for checking out if empty
//pthread_cond_t full  = PTHREAD_COND_INITIALIZER; //for checking out if full

//Message and Mailbox
int DEFAULT_MAILBOX_SIZE = 10;

typedef struct Message {
	int bytes;
	void *data;
} Message;

typedef struct Mailbox {
	Message *queue;
	int in; //place to add next element in buffer
	int out; //place to remove next element in buffer
	int cnt; //number of elements in buffer
} Mailbox;

void mb_init(Mailbox *sb, int size);
void mb_delete(Mailbox *sb);
int mb_put(Mailbox *sb, Message *msg, double con_mean, double prod_mean, FILE *f); //acts as producer
int mb_get(Mailbox *sb, Message *msg); //acts as consumer
int mb_count(Mailbox *sb);
int mb_maximum(Mailbox *sb);
int mb_isfull(Mailbox *sb, int bufsize); //C doesn't have bool functions - 0 is false, 1 is true
int mb_isempty(Mailbox *sb); //C doesn't have bool functions - 0 is false, 1 is true


//GLOBAL VARIABLES
//producer variables
double exp_prod; //random variable for sleeping for producer
//consumer variables
double exp_con; //random variable for sleeping for consumer
int put_cnt = 0;

int main( int argc, char *argv[] ) {
	long int seed = 0; //for random_seed
	int i; //for for-loop
	double x; //for rand num gen
	long nx; //for holding nanoseconds
	int iter;
   	double mean;
	double diff;
	double timeStart;//variable for starting time
	double timeEnd;//variable for ending time
		
	//argument variables
	double con_mean, prod_mean;
	int buffer_size, run_time;
	
	//create the structs
	struct Mailbox myMailbox;	
	struct Message msg;
	
	//create file to write exponential values to
	FILE *pLog = CreateLogFile();
		
	//get arguments
	con_mean = (float)atof(argv[1]);
	prod_mean = (float)atof(argv[2]);
	buffer_size = atoi(argv[3]);
	run_time = atoi(argv[4]);             

	//initialize things for your structs
	msg.bytes = sizeof(int);
	msg.data = malloc(sizeof(float));
	mb_init(&myMailbox,sizeof(float));	

	setlocale(LC_NUMERIC, "");

	if ( argc != 5 ) /* argc should be 5 for correct execution */
   	{
       		printf("Sorry! Looking for five arguments!\n");
       		exit(0);
   	}
    	else {
		//make mean a floating type and iter an integer
		mean = (float)atof(argv[2]);
		iter = atoi(argv[1]);
		
		//show what the arguments recieved are
		printf("The consumer's mean is: %f\n", con_mean);
		printf("The producer's mean is: %f\n", prod_mean);
		printf("The buffer size is: %i\n", buffer_size);
		printf("The run time is: %i\n", run_time);

		random_seed(seed);
		timeStart = format_time(); //get starting time
		printf(", Start Time\n");
		while(diff < run_time) {
			int n = rand() % 2; //set n as a random number to decide whether to produce or consumer
			if(n == 1){
				if(mb_isfull(&myMailbox, buffer_size) == 1){
					printf("Mail box is full\n");
				} else {	
					timeEnd = format_time();//display time prior to next sleep, save ending sec value
					printf(", Producer");
					int count = mb_count(&myMailbox); //find the number of elements in the mailbox
					printf(", %i", count);
					mb_put(&myMailbox,&msg,con_mean,prod_mean,pLog); //produce a random number for the consumer to read
				}
			} else if(n == 0){
				if(mb_isempty(&myMailbox) == 1){
					printf("Mailbox is empty\n");
				} else {
				//	mb_get(&myMailbox,&msg);
					nsleep(nx); //sleep for the given nanosecond value
					timeEnd = format_time();//display time prior to next sleep, save ending sec value
					printf(", Consumer");
					int count = mb_count(&myMailbox); //find the number of elements in the mailbox
					printf(", %i", count);
					mb_get(&myMailbox,&msg); //consume an element from the mailbox
				}//else
			} //else if
			diff = timeEnd - timeStart; //used to calculate the difference between starting and ending time
		}//for loop
		printf("\nThe total run time = %.9lf\n", diff);
	}//else
	fclose(pLog);
}//main

void random_seed(long int seed) {
	if(seed > 0)
		srand(seed);
	else if(seed == 0)
		srand(time(NULL));
}

double random_uniform() {
	//return numbers between 0 and 1
	return (double )rand()/(RAND_MAX+1.0);
}

double random_exponential(double mean) {
	//returns numbers based on exp distr given mean
	double x = 1.0/mean; //inverse lambda

	double u; //this will be my uniform random variable
 	double exp_value;

  	// Pull a uniform random number (0 < z < 1)
  	do
	{
       	u = random_uniform();
    }
    while ((u == 0) || (u == 1));
    exp_value = -x * log(u);

    return(exp_value);
}

double format_time(){

	time_t rawtime;
   	struct tm * timeinfo;
	struct timespec ts;

   	time ( &rawtime );
   	timeinfo = localtime ( &rawtime );
	clock_gettime(CLOCK_REALTIME, &ts);
   	printf("%d/%d/%d %d:%d:%d.%.9ld",timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ts.tv_nsec);

   	return ((double)(timeinfo->tm_sec + (double)ts.tv_nsec / (double)TEN_TO_THE_NINTH));
}

int nsleep(long nsec){
	struct timespec sleepVal = {0};
    	sleep(nsec/TEN_TO_THE_NINTH); //sleep for second value
	sleepVal.tv_nsec = (long)(nsec % TEN_TO_THE_NINTH); //set value for remaining nanoseconds
	return nanosleep(&sleepVal, NULL);
}

void mb_init(Mailbox *sb, int size){
	size = DEFAULT_MAILBOX_SIZE;
	sb->queue=(Message*)malloc(sizeof(Message));	
	sb->queue->bytes = size;
	sb->queue->data = malloc(sizeof(double));
	sb->in = 0;
        sb->out = 0;
        sb->cnt = 0;
}

int mb_put(Mailbox *sb, Message *msg, double con_mean, double prod_mean, FILE *f){
	//actions of the producer	
	pthread_mutex_lock(&mutex);	// lock access to the queue
	
	double nx;
	sb->in++; // inc. nb->queue->data);mber of messages sent
	sb->cnt += sb->queue->bytes; // add to elements in buffer
	
	//produce random generated values
	exp_prod = random_exponential(prod_mean);
        exp_con = random_exponential(con_mean);	
	
	fprintf(f,"%f\n",exp_prod);
		
	//extract value and put into queue	
	*(double*)(sb->queue->data) = exp_con;
	
	nx = (exp_prod * TEN_TO_THE_NINTH); //convert x to nsec
	printf(", Producer will now sleep for %'lf nsec\n", nx);
	nsleep(nx); //sleep for the given nanosecond value	

	pthread_mutex_unlock(&mutex);	// unlock queue
}

int mb_get(Mailbox *sb, Message *msg){
	//actions of the consumer
	pthread_mutex_lock(&mutex);		// lock access to the queue
	double exp_con_get;
	double nx;
	sb->in--;
	sb->out++;  // inc. number of messages received
	sb->cnt -= sb->queue->bytes; // add to elements in buffer
	exp_con_get = * (double*)sb->queue->data;
	
	//sleep for consumer
	nx = (exp_con_get * TEN_TO_THE_NINTH); //convert x to nsec
	printf(", Consumer will now sleep for %'lf nsec\n", nx);        
	nsleep(nx); //sleep for the given nanosecond value	
		
	pthread_mutex_unlock(&mutex);   // unlock queue
}

int mb_count(Mailbox *sb) {
	
	return sb->in;	
}

int mb_isfull(Mailbox *sb, int bufsize) { //C doesn't have bool functions - 0 is false, 1 is true
		
	if (sb->in >= bufsize) // check if buffer is full
	{
		return 1; // true
	}
	else {
		return 0; // false
	}
}


int mb_isempty(Mailbox *sb) { //C doesn't have bool functions - 0 is false, 1 is true
	
	if (sb->in == 0) // check if number of elements in the buffer is 0
	{
		return 1; // true
	}
	else {
		return 0; // false
	}
}

FILE *CreateLogFile() 
{
    return fopen("randNum.txt","w"); // allocates a FILE object and returns a pointer to it
}
















