#include "mythread.h"
#include<stdio.h>
#include<ucontext.h>
#include<stdlib.h>

void display_ready();
void create();
void enqueue_ready(int data);
void enqueue_blocked(int data);
void enqueue_blocked_join(int data, int child_thread_number);
int dequeue_ready();
void blocked_to_ready(int parentid);
void blocked_join_to_ready(int parentid);
void display_ready();
void display_blocked();
void display_blocked_join();
  
int flag_ready = 0;
int ready_queue_count = 0;
int blocked_queue_count = 0;
int blocked_join_queue_count = 0;
ucontext_t uctx_main;
/*thread_number denotes number of threads created and assigns no to each thread*/
int thread_number = 0;
int active_thread_number; //stores the information of currently active thread

typedef struct thread_structure{
  int tid; //thread number
  ucontext_t uctx_thread;
  int parent_id; //stores parent id of the thread
  int child_id[100]; //stores child id's of the thread
  int child_count; //no of child who haven't yet called exit
  int join_flag; //if join called on a particular thread then stores the thread number else stores -1
}mythread;

/*thread_arr contains information of each created thread*/
mythread thread_arr[100000];
mythread return_thread;

/*
QUEUE 
front_ready-rear_ready : READY QUEUE
front_blocked-rear_blocked : BLOCKED QUEUE FOR JOINALL
front_join_queue-rear_join_queue : BLOCKED QUEUE FOR JOIN CALL
*/
struct node
{
  int thread_number;
  struct node *ptr;
}*temp, *front1, *front_ready, *rear_ready, *front_blocked, *rear_blocked, *front_blocked_join, *rear_blocked_join;




//Used for creating new Semaphore in mysemaphore init
int semaphore_number = 0;
/*Structure of Semaphore*/
typedef struct semaphore_struct{
  int sem_number; //stores the semaphore number
  int current_value; //stores the no of threads allowed to enter the semaphore
  int status; //if status = 1, SEM is active, if status = -1 SEM is destroyed
  struct node * front; //points to front of SEM QUEUE
  struct node * rear; //points to rear of SEM QUEUE
  int queue_count; //stores the count of SEM BLOCKED QUEUE
}mysemaphore;
mysemaphore * S[10000];
void enqueue_sem_blocked(mysemaphore * mysem);
void sem_blocked_to_ready(mysemaphore * mysem);
void display_sem_blocked(mysemaphore * mysem);



MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
  thread_number++;
  char *a = malloc(8192);
  getcontext(&thread_arr[thread_number].uctx_thread);
  thread_arr[thread_number].uctx_thread.uc_stack.ss_sp = a;
  thread_arr[thread_number].uctx_thread.uc_stack.ss_size = 8192;
  //thread_arr[thread_number].uctx_thread.uc_link = &thread_arr[active_thread_number].uctx_thread;  
  thread_arr[thread_number].tid = thread_number; //state the thread number
  thread_arr[thread_number].parent_id = active_thread_number; //parent_id = -1 states init thread
  int i;
  for(i=0; i<=99; i++){  //initiates the list of children i.e no children
    thread_arr[thread_number].child_id[i] = -1;
  }
  thread_arr[thread_number].child_count = 0;
  thread_arr[thread_number].join_flag = -1; //join flag initialised to -1

  int setchild = 0; //used to set child...searches for non -1 entry in child_id and then sets thread with thread_number as child in child_id array
  while((thread_arr[active_thread_number].child_id[setchild] != -1) && (setchild < 99))
    setchild++;
  thread_arr[active_thread_number].child_id[setchild] = thread_number; //thread with thread_number added as child in the active_thread child array
  thread_arr[active_thread_number].child_count ++;

  /*
  printf("CHILDREN of %d :",active_thread_number);
  for(int j = 0; (j < 9) && (thread_arr[active_thread_number].child_id[j] != -1); j++)
    printf("%d ", thread_arr[active_thread_number].child_id[j]);
  printf("\n");
  */

  //printf("PARENT : %d, CHILD : %d\n", active_thread_number, thread_arr[active_thread_number].child_id[setchild]);

  
  makecontext(&thread_arr[thread_number].uctx_thread, start_funct, 1, args);
  ///active_thread_number = thread_number; //now thread with value thread_number will become the active thread

  enqueue_ready(thread_number);

  return_thread = thread_arr[thread_number];
  return &return_thread;
  // return thread_arr[thread_number];

  ///swapcontext(&thread_arr[prev_active_thread_number].uctx_thread, &thread_arr[thread_number].uctx_thread);

  //active_thread_number = prev_active_thread_number; //to reassign the active thread number to parent as this thread will return to parent
}


void MyThreadInit(void(*start_funct)(void *), void *args)
{
  /*Initialize READY QUEUE and BLOCKED QUEUE to NULL*/
  //printf("Initialised both the QUEUES1\n");
  create();
  //printf("Initialised both the QUEUES\n");
  
  char *a = malloc(8192);
  //Modifying the context
  getcontext(&thread_arr[thread_number].uctx_thread);
  thread_arr[thread_number].uctx_thread.uc_stack.ss_sp = a;
  thread_arr[thread_number].uctx_thread.uc_stack.ss_size = 8192;
  
  thread_arr[thread_number].tid = thread_number; //state the thread number
  thread_arr[thread_number].parent_id = -1; //parent_id = -1 states init thread
  int i;
  for(i=0; i<=99; i++){  //initiates the list of children i.e no children
    thread_arr[thread_number].child_id[i] = -1;
  }
  thread_arr[thread_number].child_count = 0;
  
  /*executing function*/
  makecontext(&thread_arr[thread_number].uctx_thread, start_funct, 1, args);
  active_thread_number = thread_number; //now thread with value thread_number will become the active thread
  swapcontext(&uctx_main, &thread_arr[thread_number].uctx_thread);
}


void MyThreadYield(void){
  //printf("BEFORE YIELD\n");
  //printf("READY QUEUE COUNT %d\n", ready_queue_count);
  
  //Case 1: If ready queue is empty, then continue executing the current thread
  if(ready_queue_count == 0){
  }

  //Case 2: If ready queue is not empty, then swapcontext
  else{
    // display_ready();
    int prev_active_thread_number = active_thread_number;
    active_thread_number = dequeue_ready();
    enqueue_ready(prev_active_thread_number);
    //printf("AFTER YIELD\n");
    //display_ready();
    //printf("READY QUEUE COUNT : %d\n", ready_queue_count);
    swapcontext(&thread_arr[prev_active_thread_number].uctx_thread, &thread_arr[active_thread_number].uctx_thread);
  }
}

int MyThreadJoin(MyThread thread){
  mythread * new_thread = (mythread *)thread;

  //Case 0: If null pointer is passed, then return 0
  if(new_thread == 0)
    return -1;
  
  int thread_id = new_thread->tid;
  //printf("THREAD ID: %d\n", thread_id);
  //printf("JOIN called ON: %d\n", thread_id);
  //printf("JOIN called BY: %d\n", active_thread_number);
    int check_child = -1; //check if parent has called join for child only or not
  for(check_child = 0; (check_child < 99) && (thread_arr[active_thread_number].child_id[check_child] != thread_id); check_child++){
  }

  //Case 1: Join not called on child ->check_child=99 i.e child not fould in parent list
  //Case 2: Join called on child but child has already exited
  if(check_child == 99){
    return -1;
  }

  //Case 3: Child not exited -> block parent and return 1
  if((check_child >= 0) && (check_child < 99)){
    thread_arr[active_thread_number].join_flag = thread_id;
    enqueue_blocked_join(active_thread_number, thread_id);
    // display_blocked_join();
    int prev_active_thread_number = active_thread_number;
    int new_active_thread_number = dequeue_ready();
    active_thread_number = new_active_thread_number;
    //printf("SWAP(%d, %d)\n", prev_active_thread_number, active_thread_number);
    swapcontext(&thread_arr[prev_active_thread_number].uctx_thread, &thread_arr[active_thread_number].uctx_thread);
    return 0;
      }
}

void MyThreadJoinAll(void){
  /*Check if all children have called exit*/
  //Case 1 : if there are some running child then put this thread in blocked queue
  //printf("INSIDE JOINALL thread(%d)\n", active_thread_number);
  if(thread_arr[active_thread_number].child_count != 0){
    enqueue_blocked(active_thread_number);
    int prev_active_thread_number = active_thread_number;
    //display_ready();
    //printf("COUNT of READY QUEUE %d\n", ready_queue_count);
    //display_blocked();
    int new_active_thread_number = dequeue_ready();
    active_thread_number = new_active_thread_number;
    //printf("SWAP(%d, %d)\n", prev_active_thread_number, active_thread_number);
    swapcontext(&thread_arr[prev_active_thread_number].uctx_thread, &thread_arr[active_thread_number].uctx_thread);
    //printf("INSIDE JOINALL\n");
  }
  //printf("CHILDREN of %d :",active_thread_number);
  int j;

  /*
  for(j = 0; (j < 99) && (thread_arr[active_thread_number].child_id[j] != -1); j++)
    printf("%d ", thread_arr[active_thread_number].child_id[j]);
  printf("\n");

  printf("ACTIVE THREAD INSIDE JOINALL id: %d\n", active_thread_number);
  */   
  //Case 2 : if no currently active children then do nothing and let the current thread to execute
}

void MyThreadExit(void){

  /*USEFUL
  printf("\nEXIT called by %d\n", active_thread_number);
  display_ready();
  printf("READY QUEUE COUNT %d\n", ready_queue_count);
  display_blocked();
  */

  if(active_thread_number == 0){
    //If READY QUEUE is EMPTY then exit
    if(ready_queue_count == 0)
      setcontext(&uctx_main);
    else{
      MyThreadYield();
      MyThreadExit();
    }
  }
  else{
    int current_active_thread_number = active_thread_number; //currently active thread
    int parentid = thread_arr[current_active_thread_number].parent_id; //parent of currently active thread
    thread_arr[parentid].child_count--;

    int j;
    for(j = 0; (j < 99) && (thread_arr[parentid].child_id[j] != current_active_thread_number); j++){
    }
    thread_arr[parentid].child_id[j] = -1;
    //      printf("%d ", thread_arr[active_thread_number].child_id[j]);

    
    //printf("PARENT id : %d  CHILD count %d\n",thread_arr[current_active_thread_number].parent_id, thread_arr[parentid].child_count);
    int setchild = 0;
    /*If parent's child_count=0, remove parent from block queue and put it in ready queue*/

    /*Checking for JOIN QUEUE*/
    if(thread_arr[parentid].join_flag == current_active_thread_number){
      blocked_join_to_ready(parentid);
    }

    /*Checking for JOINALL QUEUE*/
    else{
      if(thread_arr[parentid].child_count == 0){
	//printf("Moving from BLOCKED_TO_READY\n");
	blocked_to_ready(parentid);

	/*USEFUL
	printf("%d MOVED from blocked to ready\n", parentid);
	display_ready();
	printf("READY QUEUE COUNT %d\n", ready_queue_count);
	display_blocked();
	*/
      }
    }
    int ready_dequeue = dequeue_ready();

    /*USEFUL
    printf("READY_DEQUEUE(%d)\n", ready_dequeue);
    display_ready();
    printf("READY QUEUE COUNT %d\n", ready_queue_count);
    display_blocked();
    */
    int  prev_active_thread_number = active_thread_number;
    active_thread_number = ready_dequeue;
    //printf("NEW ACTIVE THREAD id %d\n", active_thread_number);
    setcontext(&thread_arr[active_thread_number].uctx_thread);
  }  
}


MySemaphore MySemaphoreInit(int initialValue){

  //Case 1: If initialValue is invalid then return NULL
  if(initialValue < 0)
    return NULL;

  //Case 2: Create semaphore and set the initial value
  else{
    S[semaphore_number] = (mysemaphore *)malloc(sizeof(mysemaphore));
    // S[semaphore_number]->initial_value = initialValue;
    S[semaphore_number]->sem_number = semaphore_number;
    S[semaphore_number]->current_value = initialValue;
    S[semaphore_number]->status = 1;
    S[semaphore_number]->front = S[semaphore_number]->rear = NULL;
    S[semaphore_number]->queue_count = 0;

    //printf("SEM(%d) INITIALIZED\n", S[semaphore_number]->sem_number);
    // printf("SEM: %d\n", S[semaphore_number]);
    //printf("SEM ADDRESS: %d\n", &S[semaphore_number]);
    //printf("INIT CURRENT VALUE: %d\n", S[semaphore_number]->current_value);
    semaphore_number ++;

    return S[semaphore_number - 1];
  }
}

void MySemaphoreSignal(MySemaphore sem){
  mysemaphore * mysem = (mysemaphore *) sem;

  //Case 0: If an destroyed SEM or null pointer is passed, then do not do anything and continue the program
  if(mysem == 0){
    //printf("SEM is NULL POINTER\n");
  }
  //Destroyed SEM
  if(mysem->status == -1){
    //printf("SEM(%d) is already DESTROYED\n", mysem->sem_number);
  }
  
  //Case 1: SEM is active and valid
  else{

    //printf("SIGNAL called on SEM(%d)\n", mysem->sem_number);
    //display_sem_blocked(mysem);
    
    //Case 1.1: If current_value > 0 then no one is waiting and so just increment the signal and continue
    if(mysem->current_value > 0)
      mysem->current_value ++;

    //Case 2.1: If current_value = 0
    else{
      //Case 2.1.1: If SEM BLOCKED QUEUE is empty, then just increment the current_value and continue
      if(mysem->rear == NULL)
	mysem->current_value ++;


      //Case 2.1.2: If SEM BLOCKED QUEUE is not-empty, then dequeue from SEM BLOCKED QUEUE and enqueue in READY queue
      else
	sem_blocked_to_ready(mysem);
    }
  }

}

void MySemaphoreWait(MySemaphore sem){
 
  mysemaphore * mysem = (mysemaphore *) sem;
  //Case 0: If an destroyed SEM or null pointer is passed, then do not do anything and continue the program
  if(mysem == 0){
    //printf("SEM is NULL POINTER\n");
  }
  //Destroyed SEM
  if(mysem->status == -1){
    //printf("SEM(%d) is already DESTROYED\n", mysem->sem_number);
  }
    
    
  //Case 1: SEM is active and valid
  else{

    //printf("WAIT called on SEM(%d)\n", mysem->sem_number);
    //display_sem_blocked(mysem);
    //printf("CURRENT VALUE %d\n", mysem->current_value);
    
    //Case 1.1: If current_value > 0, allow the thread to enter CS as no one is waiting
    if(mysem->current_value > 0){
      mysem->current_value --;
    }

    //Case 1.2: If current_value = 0, then creade node and add the thread at the end of QUEUE and then switch context
    else{
      // printf("INSIDE ELSE\n");
      //display_ready();
      enqueue_sem_blocked(mysem);
      //display_sem_blocked(mysem);
      int new_active_thread_number = dequeue_ready();
      int prev_active_thread_number = active_thread_number;
      active_thread_number = new_active_thread_number;
      swapcontext(&thread_arr[prev_active_thread_number].uctx_thread, &thread_arr[new_active_thread_number].uctx_thread);
    }
  }
  
}

int MySemaphoreDestroy(MySemaphore sem){
  mysemaphore * destroy_sem = (mysemaphore *)sem;

  //Case 0: If destroy called on NULL SEM, then do not do anything
  if(destroy_sem == 0){
    //printf("SEM is NULL POINTER\n");
    return -1;
  }

  //Case 1: SEM is already destroyed
  if(destroy_sem->status == -1){
    //printf("SEM(%d) is already DESTROYED\n", destroy_sem->sem_number);
    return -1;
  }

  //printf("INSIDE DESTROY\n");
  //display_sem_blocked(destroy_sem);
  
  //Case 2: If SEM QUEUE count = 0, the destroy semaphore i.e set status = -1
  if(destroy_sem->queue_count == 0){
    destroy_sem->status = -1;
    //printf("SEM(%d) DESTROYED\n", destroy_sem->sem_number);
    return 0;
  }

  //Case 3: If SEM QUEUE count != 0, then return -1
  else{
    //printf("SEM(%d) NOT DESTROYED\n", destroy_sem->sem_number);
    return -1;
  }
}

void create()
{
  front_ready = rear_ready = NULL; //initialize READY QUEUE to NULL
  front_blocked = rear_blocked = NULL; //initialize joinall blocked QUEUE to NULL
  front_blocked_join = rear_blocked_join = NULL; //initialize join blocked QUEUE to NULL
}

void enqueue_ready(int data)
{
  //printf("ENQUEUE_READY(%d)\n", data);
  if (rear_ready == NULL)
    {
      rear_ready = (struct node *)malloc(1*sizeof(struct node));
      rear_ready->ptr = NULL;
      rear_ready->thread_number = data;
      front_ready = rear_ready;
    }
  else
    {
      temp=(struct node *)malloc(1*sizeof(struct node));
      rear_ready->ptr = temp;
      temp->thread_number = data;
      temp->ptr = NULL;
 
      rear_ready = temp;
    }
  ready_queue_count++;
}

void enqueue_blocked(int data)
{
  if (rear_blocked == NULL)
    {
      rear_blocked = (struct node *)malloc(1*sizeof(struct node));
      rear_blocked->ptr = NULL;
      rear_blocked->thread_number = data;
      //rear_blocked->thread_child_number = -1; //for joinall the child number=-1 as the thread is waiting for all childs
      front_blocked = rear_blocked;
    }
  else
    {
      temp=(struct node *)malloc(1*sizeof(struct node));
      rear_blocked->ptr = temp;
      temp->thread_number = data;
      //temp->thread_child_number = -1; //for joinall the child number=-1 as the thread is waiting for all childs
      temp->ptr = NULL;
 
      rear_blocked = temp;
    }
  blocked_queue_count++;
}
 
 
void enqueue_blocked_join(int data, int child_thread_number)
{
  if (rear_blocked_join == NULL)
    {
      rear_blocked_join = (struct node *)malloc(1*sizeof(struct node));
      rear_blocked_join->ptr = NULL;
      rear_blocked_join->thread_number = data;
      //rear_blocked->thread_child_number = -1; //for joinall the child number=-1 as the thread is waiting for all childs
      front_blocked_join = rear_blocked_join;
    }
  else
    {
      temp=(struct node *)malloc(1*sizeof(struct node));
      rear_blocked_join->ptr = temp;
      temp->thread_number = data;
      //temp->thread_child_number = -1; //for joinall the child number=-1 as the thread is waiting for all childs
      temp->ptr = NULL;
 
      rear_blocked_join = temp;
    }
  blocked_join_queue_count ++;
}

int dequeue_ready()
{
  front1 = front_ready;
 
  if (front1 == NULL)
    {
      //printf("\n Error: Trying to display elements from empty queue");
      return 0;
    }
  else
    if (front1->ptr != NULL)
      {
	int data = front1->thread_number;
	front1 = front1->ptr;
	//free(front_ready);
	front_ready = front1;
	ready_queue_count --;
	return data;
      }
    else
      {
	int data = front1->thread_number;
	//printf("\n Dequed value : %d", front_ready->thread_number);
	free(front_ready);
	front_ready = NULL;
	rear_ready = NULL;
	ready_queue_count --;
	return data;
      }
 
}

void blocked_to_ready(int parentid)
{
  front1 = front_blocked;
  
  if (front1 == NULL)
    {
    }
  else{
    if(front1->thread_number == parentid){
      if(front_blocked == rear_blocked){
	front_blocked = NULL;
	rear_blocked = NULL;
      }
      front1 = front1->ptr;
      //free(front_blocked);
      //printf("1\n");
      enqueue_ready(parentid);
      //front_blocked = front1;
      // ready_queue_count++;
      blocked_queue_count--;
    }
    else{
      while((front1->ptr != NULL) && (front1->ptr->thread_number != parentid))
	front1 = front1->ptr;
      if(front1->ptr->thread_number == parentid){
	if(front1->ptr == rear_blocked){
	  // printf("2\n");
	  enqueue_ready(parentid);
	  // ready_queue_count++;
	  blocked_queue_count--;
	  rear_blocked = front1;
	  rear_blocked->ptr = NULL;
	}
	else{
	  enqueue_ready(parentid);
	  temp = front1->ptr;
	  front1->ptr = front1->ptr->ptr;
	  free(temp);
	  //ready_queue_count++;
	  blocked_queue_count--;
	}
      }
    }
  }
}

void blocked_join_to_ready(int parentid)
{
  front1 = front_blocked_join;
  
  if (front1 == NULL)
    {
    }
  else{
    if(front1->thread_number == parentid){
      if(front_blocked_join == rear_blocked_join){
	front_blocked_join = NULL;
	rear_blocked_join = NULL;
      }
      front1 = front1->ptr;
      //free(front_blocked);
      //printf("1\n");
      enqueue_ready(parentid);
      //front_blocked = front1;
      // ready_queue_count++;
      blocked_join_queue_count--;
    }
    else{
      while((front1->ptr != NULL) && (front1->ptr->thread_number != parentid))
	front1 = front1->ptr;
      if(front1->ptr->thread_number == parentid){
	if(front1->ptr == rear_blocked_join){
	  // printf("2\n");
	  enqueue_ready(parentid);
	  // ready_queue_count++;
	  blocked_join_queue_count--;
	  rear_blocked_join = front1;
	  rear_blocked_join->ptr = NULL;
	}
	else{
	  enqueue_ready(parentid);
	  temp = front1->ptr;
	  front1->ptr = front1->ptr->ptr;
	  free(temp);
	  //ready_queue_count++;
	  blocked_join_queue_count--;
	}
      }
    }
  }
}


void display_ready()
{printf("READY QUEUE: ");
  front1 = front_ready;
 
  if ((front1 == NULL) && (rear_ready == NULL))
    {
      printf("Queue is empty\n");
      return;
    }
  while (front1 != rear_ready)
    {
      printf("%d ", front1->thread_number);
      front1 = front1->ptr;
    }
  if (front1 == rear_ready)
    printf("%d", front1->thread_number);
  printf("\n");
}

void display_blocked()
{
  printf("BLOCKED QUEUE: ");
  front1 = front_blocked;
 
  if ((front1 == NULL) && (rear_blocked == NULL))
    {
      printf("Queue is empty\n");
      return;
    }
  while (front1 != rear_blocked)
    {
      printf("%d ", front1->thread_number);
      front1 = front1->ptr;
    }
  if (front1 == rear_blocked)
    printf("%d", front1->thread_number);
  printf("\n");
}

void display_blocked_join()
{printf("BLOCKED JOIN QUEUE: ");
  
  front1 = front_blocked_join;
 
  if ((front1 == NULL) && (rear_blocked_join == NULL))
    {
      printf("Queue is empty\n");
      return;
    }
  while (front1 != rear_blocked_join)
    {
      printf("%d ", front1->thread_number);
      front1 = front1->ptr;
    }
  if (front1 == rear_blocked_join)
    printf("%d", front1->thread_number);

  printf("\n");
}
 
 
void enqueue_sem_blocked(mysemaphore * mysem)
{
  if (mysem->rear == NULL)
    {
      mysem->rear = (struct node *)malloc(1*sizeof(struct node));
      mysem->rear->ptr = NULL;
      mysem->rear->thread_number = active_thread_number;
      mysem->front = mysem->rear;
    }
  else
    {
      temp=(struct node *)malloc(1*sizeof(struct node));
      mysem->rear->ptr = temp;
      temp->thread_number = active_thread_number;
      temp->ptr = NULL;
 
      mysem->rear = temp;
    }
  mysem->queue_count ++;
}

void sem_blocked_to_ready(mysemaphore * mysem)
{
  int threadid = mysem->front->thread_number;
  front1 = mysem->front;

  //Case 1: Only 1 element in the queue
  if(mysem->front == mysem->rear){
    mysem->front = NULL;
    mysem->rear = NULL;
  }
  else{
    //Case 2: Only 2 elements
    if(front1->ptr == mysem->rear){
      mysem->rear = front1;
      mysem->rear->ptr = NULL;
    }
    //Case 3: Many elements
    else{
      temp = front1->ptr;
      front1->ptr = front1->ptr->ptr;
      free(temp);
    }
  }
  enqueue_ready(threadid);
  mysem->queue_count --;
}

void display_sem_blocked(mysemaphore * mysem)
{
  
  printf("SEM(%d) BLOCKED QUEUE: ", mysem->sem_number);
  
  front1 = mysem->front;
 
  if ((front1 == NULL) && (mysem->rear == NULL))
    {
      printf("Queue is empty\n");
      return;
    }
  while (front1 != mysem->rear)
    {
      printf("%d ", front1->thread_number);
      front1 = front1->ptr;
    }
  if (front1 == mysem->rear)
    printf("%d", front1->thread_number);

    printf("\n");
}
 
