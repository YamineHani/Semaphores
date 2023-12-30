#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h> 
#include <semaphore.h> 
#include <unistd.h>
#include <signal.h>

#define N 5
#define BUFFER_SIZE 5

sem_t mutex,full,empty,counter;
int shared_counter = 0;
int i = 0;
int j = 0;

typedef struct Node {
    int data;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int capacity;
    int size;
} Queue;

Queue *queue;

Queue* constructorQueue(int capacity) {
    Queue *queue = malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = queue->tail = NULL;
    return queue;
}

Node* constructorNode(int data)
{
   Node *node = malloc(sizeof(Node));
   node->data = data;
   node->next = NULL;
   return node;
}

void enqueue(Queue *queue,int data){
   if(queue->size < queue->capacity){
       Node *node = constructorNode(data);
       //if first element
       if(queue->head == NULL){
           queue->head = node;
       } else{
           queue->tail->next = node;
       }
       queue->tail = node;
       queue->tail->next = queue->head;
       queue->size += 1;
   }
}

int dequeue(Queue *queue){
   if (queue->size > 0) {
        int data;
        if(queue->head == queue->tail){
            data = queue->head->data;
            free(queue->head);
            queue->head = NULL;
            queue->tail = NULL;
        }
        else{
            Node *n = queue->head;
            data = n->data;
            queue->head = queue->head->next;
            queue->tail->next = queue->head;
            free(n);
        }
        queue->size--;
        return data;
    } 
}

//returns 1 if queue is empty and 0 otherwise
int isEmpty(Queue *queue){
   if(queue->size == 0){
       return 1;
   }
   return 0;
}

void freeQ(Queue *queue) {
   while (queue->head != NULL) {
       Node* temp = queue->head;
       queue->head = queue->head->next;
       free(temp);
   }
   queue->tail = NULL;
   free(queue);
}



void* mCollector_thread(void *args) 
{ 
    while (1)
    {
        int sem_value;
		sem_getvalue(&empty,&sem_value);
        if (sem_value == BUFFER_SIZE) {
            printf("Collector thread: nothing is in the buffer! \n\n");
        }
		//wait 
		sem_wait(&full);
		sem_wait(&mutex); 
		/*          ENTERS CRITICAL SECTION       */
        dequeue(queue);
        /*          EXITS CRITICAL SECTION       */
		sem_post(&mutex); 
        sem_getvalue(&empty,&sem_value);
        printf("Collector thread: reading from buffer at position %d\n\n",i);
        i = (i + 1)%BUFFER_SIZE;
		sem_post(&empty); 

		//sleep after 
		sleep(random() % 6 + 1); 
    }
    
	
} 

void* mCounter_thread(void *args) 
{ 
    int number = *(int *)args;
    while (1) {
        printf("Counter thread %d: received a message\n\n", number);
        printf("Counter thread %d: Waiting to write\n\n", number);

        // Lock the mutex to ensure exclusive access to the shared counter
        sem_wait(&counter);

        /*          ENTERS CRITICAL SECTION       */
        shared_counter++;// Add one to the counter
        /*          EXITS CRITICAL SECTION       */

        printf("Counter thread %d: now adding to counter,counter value=%d\n\n", number, shared_counter);
        // Unlock the mutex to allow other threads to access the counter
        sem_post(&counter);

        sleep(random() % 3 + 1);  // Sleep for a random period 
    }
	
} 

void* mMonitor_thread(void *args) 
{ 
	while (1) {
        printf("Monitor thread: waiting to read counter\n\n");
        // Wait on the semaphore to enter the critical section
        sem_wait(&counter);

        /*          ENTERS CRITICAL SECTION       */
        printf("Monitor thread: reading a count value of %d\n\n", shared_counter);
        int saved_counter_value = shared_counter;  
        shared_counter = 0;  // Reset the counter to 0
        /*          EXITS CRITICAL SECTION       */


        // Signal the semaphore to exit the critical section
        sem_post(&counter);

        int sem_value;
		sem_getvalue(&full,&sem_value);
        if (sem_value == BUFFER_SIZE) {
            printf("Monitor thread: Buffer full! \n\n");
        }
        //wait 
		sem_wait(&empty);
		sem_wait(&mutex);
		/*          ENTERS CRITICAL SECTION       */
        enqueue(queue, saved_counter_value);
        /*          EXITS CRITICAL SECTION       */
		sem_post(&mutex); 
		//signal 
		sem_post(&full); 
        //sem_getvalue(&full,&sem_value);
        printf("Monitor thread: Writing to buffer at position %d\n\n",j);
        j = (j+1)%BUFFER_SIZE;
        sleep(random() % 4 + 1);  
    }
} 
void intHandler(int dummy) {
	// Destroy the semaphore 
    sem_destroy(&counter);
	sem_destroy(&mutex); 
	sem_destroy(&full); 
	sem_destroy(&empty); 
    freeQ(queue);
	exit(0);
}
int main() 
{ 
    signal(SIGINT, intHandler);
    queue = constructorQueue(BUFFER_SIZE);
    pthread_t mCounter[N], mCollector, mMonitor;

    sem_init(&counter, 0, 1);
    sem_init(&mutex, 0, 1); 
	sem_init(&full, 0, 0); 
	sem_init(&empty, 0, BUFFER_SIZE);

    // Create threads
    int threadID[N];
    for (int i = 0; i < N; i++) {
        threadID[i] = i+1;
        pthread_create(&mCounter[i], NULL, mCounter_thread,&threadID[i]);
    }
    pthread_create(&mMonitor, NULL, mMonitor_thread, NULL);
    pthread_create(&mCollector, NULL, mCollector_thread, NULL);


    // Wait for all threads to finish
    for (int i = 0; i < N; i++) {
        pthread_join(mCounter[i], NULL);
    }
    pthread_join(mMonitor, NULL);
    pthread_join(mCollector, NULL);
    
    //destroy semaphore
    sem_destroy(&mutex); 
	sem_destroy(&full); 
	sem_destroy(&empty); 
    sem_destroy(&counter); 
    freeQ(queue);
} 