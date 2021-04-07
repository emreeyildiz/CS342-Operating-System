#include <stdlib.h> //
#include <stdio.h> //
#include <unistd.h> //
#include <sys/types.h> //
#include <string.h> //
#include <errno.h> //
#include <sys/time.h> //
#include <pthread.h> //
#include <mqueue.h> //
#include <sys/wait.h> //
#include <math.h> ///

int * fileArray;
double *vruntimeArr;
unsigned long *avgWaitingTime;

struct node {
  struct node *next;
  int data;
  long t_index;
  long b_index;
  long b_length;
  long gen_time;
};

struct linkedList {
  struct node *head;
  struct node *tail;
  int count;
};

void linkedList_init(struct linkedList *q){
  q->count = 0;
  q->head = NULL;
  q->tail = NULL;
}

void linkedList_insert(struct linkedList *q, struct node *newNode){
  if(q->count == 0){
    q->head = newNode;
    q->tail = newNode;
  }
  else{
    q->tail->next = newNode;
    q->tail = newNode;
  }
  q->count++;
}

struct node * linkedList_retrieve(struct linkedList *q, char *alg){
  struct node * retrievedNode;
  int flag = 0;
  retrievedNode = q->head;
  if(strcmp("FCFS", alg) == 0){
    q->head = q->head->next;
    flag = 1;
  }
  else if(strcmp("SJF", alg) == 0){
    for(struct node *cur = q->head->next ; cur != NULL ;cur = cur->next ){
      if(cur->b_length < retrievedNode->b_length){
        retrievedNode = cur;
      }
    }
  }
  else if(strcmp("PRIO", alg) == 0){
    for(struct node *cur = q->head->next ; cur != NULL ;cur = cur->next ){
      if(cur->t_index < retrievedNode->t_index){
        retrievedNode = cur;
      }
    }
  }

  else if(strcmp("VRUNTIME", alg) == 0){
    for(struct node *cur = q->head->next ; cur != NULL ;cur = cur->next ){
      if(vruntimeArr[(cur->t_index)] < vruntimeArr[(retrievedNode->t_index)]){
        retrievedNode = cur;
      }
    }
  }

  if(flag == 0){
    if(retrievedNode == q->head)
      q->head = q->head->next;
    else{
      struct node *prev;
      for(prev = q->head ; prev->next != retrievedNode ;prev = prev->next ){
      }
      if(retrievedNode == q->tail){
        q->tail = prev;
      }
      prev->next = retrievedNode -> next;
    }
  }

  q->count--;

  return (retrievedNode);
}

struct bounded_buffer {
  struct linkedList *q;
  pthread_mutex_t th_mutex_queue;
  pthread_cond_t th_cond_hasitem;   //Consumer
};

void add(struct bounded_buffer* bbp, struct node *newNode ) {
  pthread_mutex_lock(&bbp->th_mutex_queue);

  // Critical section start

  linkedList_insert(bbp->q, newNode);

  if(bbp->q->count == 1){
    pthread_cond_signal(&bbp->th_cond_hasitem);
  }

  // End of the critical section

  printf("Insertion on Tread index: %li, b_length: %lu, b_index: %li\n", newNode->t_index, newNode->b_length, newNode->b_index);
  pthread_mutex_unlock(&bbp->th_mutex_queue);
}

struct node * bb_retrieve(struct bounded_buffer *bbp, char* alg){
  struct node *qe;
  pthread_mutex_lock(&bbp->th_mutex_queue);

  // Critical section start

  while(bbp->q->count == 0){
    pthread_cond_wait(&bbp->th_cond_hasitem, &bbp->th_mutex_queue);
  }

  qe = linkedList_retrieve(bbp->q, alg);
  if(qe == NULL){
    printf("can not retrieved\n");
    exit(1);
  }
  // End of the critical section
  printf("Retrieve on Tread index: %li, b_length: %lu, b_index: %li\n", qe->t_index, qe->b_length, qe->b_index);
  pthread_mutex_unlock(&bbp->th_mutex_queue);
  return (qe);
}

//Global variables
struct bounded_buffer *buf;
int n;
int bCount;
int minB;
int avgB;
int minA;
int avgA;
char alg[16];
char infile[64];


double expDist(double x, int y){
  double random;
  double result;
  do{
    random = rand() / (RAND_MAX + 1.);
    result = -log(1-random) / x;
  }
  while(result < y);
  return result;
}

static void * worker_thread(void *arg){
  long int index;
  int i = 0;
  struct node *qe;
  index = (long int) arg;
  srand(index);
  for (i = 0; i < bCount; ++i){
    usleep((unsigned long)(expDist((1./avgA), minA)*1000));
    qe = (struct node *) malloc (sizeof(struct node));
    if(qe == NULL){
      perror("malloc failed\n");
      exit(1);
    }

    qe->next = NULL;
    qe-> t_index = index;
    qe-> b_index = (long) i+1;
    qe-> b_length = (unsigned long)expDist((1./avgB), minB);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    qe-> gen_time = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    add(buf, qe);
  }
  pthread_exit(NULL);
}
static void * schedule_thread(void *arg){
  int num = 0;
  struct node *newNode;
  unsigned long nowTime;
  while(1){
    if(num == bCount*n)
      break;
    //printf("Kaç kere n: %d\n", num);
    newNode = bb_retrieve(buf, alg); //TO DO Write for each algorithm
    struct timeval tv;
    gettimeofday(&tv, NULL);
    nowTime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    avgWaitingTime[(newNode->t_index)] = avgWaitingTime[(newNode->t_index)] + (nowTime - newNode->gen_time);
    usleep((newNode->b_length)*1000);
    vruntimeArr[(newNode->t_index)] = vruntimeArr[(newNode->t_index)] + ((newNode->b_length) * (0.7 + 0.3 * newNode->t_index));
    //printf("Kaç kere n: %d\n", num);
    free(newNode);
    num++;
  }

  pthread_exit(NULL);
}



void calculate_bCount(){
  bCount = 0;
  for (int i = 0; i < n; ++i){
    FILE *fp;
    char str[256];
    char filename[64];
    char ch;
    int lineCount = 0;
    strcpy(filename, infile);
    sprintf(str, "-%d.txt", i+1);
    strcat(filename, str);
    // printf("%s\n", filename);
    fp = fopen(filename, "r");
    if(fp != NULL){
      while(!feof(fp)){
        ch = fgetc(fp);
        if(ch == '\n'){
          bCount = bCount + 1;
          lineCount = lineCount + 1;
        }
      }
      fileArray[i] = lineCount;
      fclose(fp);
    }
    else if(fp == NULL){
      perror("File cannot be openned while calculating bCount!");
      exit(1);
    }
  }
}



static void * worker_thread_file(void *arg){
  long int index;
  int i = 0;
  struct node *qe;
  index = (long int) arg;

  FILE *fp;
  char str[256];
  char filename[64];
  strcpy(filename, infile);
  sprintf(str, "-%li.txt", index + 1);
  strcat(filename, str);
  // printf("%s\n", filename);
  fp = fopen(filename, "r");
  if(fp == NULL){
    perror("File cannot be openned in worker_thread_file\n");
    exit(1);
  }

  while(fgets(str,256,fp) != NULL){
    char *first;
    char *second;
    first = strtok(str, "\t\a\r\n, ");
    second = strtok(NULL, "\t\a\r\n, ");
    usleep((unsigned long) atoi(first)*1000 );
    qe = (struct node *) malloc (sizeof(struct node));
    if(qe == NULL){
      perror("malloc failed\n");
      exit(1);
    }
    qe->next = NULL;
    qe-> t_index = index;
    qe-> b_index = (long) i+1;
    qe-> b_length = (unsigned long) atoi(second);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    qe-> gen_time = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    add(buf, qe);
    i++;
  }
  fclose(fp);
  pthread_exit(NULL);

}


static void * schedule_thread_file(void *arg){
  int num = 0;
  struct node *newNode;
  unsigned long nowTime;
  while(1){
    if(num == bCount)
      break;
    //printf("Kaç kere n: %d\n", num);
    newNode = bb_retrieve(buf, alg); //TO DO Write for each algorithm
    struct timeval tv;
    gettimeofday(&tv, NULL);
    nowTime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    avgWaitingTime[(newNode->t_index)] = avgWaitingTime[(newNode->t_index)] + (nowTime - newNode->gen_time);
    usleep((newNode->b_length)*1000);
    vruntimeArr[(newNode->t_index)] = vruntimeArr[(newNode->t_index)] + ((newNode->b_length) * (0.7 + 0.3 * newNode->t_index));
    //printf("Kaç kere n: %d\n", num);
    free(newNode);
    num++;
  }

  pthread_exit(NULL);
}





int main(int argc, char const *argv[]) {
  /* code */

  buf = (struct bounded_buffer* )malloc(sizeof(struct bounded_buffer));
  buf->q = (struct linkedList* )malloc(sizeof(struct linkedList));
  linkedList_init(buf->q);
  pthread_mutex_init(&buf->th_mutex_queue, NULL);
  pthread_cond_init(&buf->th_cond_hasitem, NULL);

  n = atoi(argv[1]);
  if( n < 1 || n > 10)
    exit(1);

  fileArray = (int *) malloc(n * sizeof(int));
  vruntimeArr = (double *) malloc(n * sizeof(double));
  avgWaitingTime = (unsigned long *) malloc(n * sizeof(unsigned long));

  for(int i = 0; i<n; ++i){
    fileArray[i] = 1;
    vruntimeArr[i] = 0.0;
    avgWaitingTime[i] = 0;
  }

  if (argc == 8){

    bCount = atoi(argv[2]);
    minB = atoi(argv[3]);
    avgB = atoi(argv[4]);
    minA = atoi(argv[5]);
    avgA = atoi(argv[6]);
    strcpy(alg, argv[7]);
    pthread_t worktids[n];
    pthread_t stid;
    int i;
    int ret;

    for(i = 0; i < n; i++){
      fileArray[i] = bCount;
      ret = pthread_create(&worktids[i], NULL, worker_thread, (void*) (long) i);
      if(ret < 0 ) {
        perror("thread create failed\n");
        exit(1);
      }
    }

    ret = pthread_create(&stid, NULL, schedule_thread, (void*) NULL);
    if(ret < 0 ) {
      perror("thread create failed\n");
      exit(1);
    }

    for(i = 0; i < n; i++){
      pthread_join(worktids[i], NULL);
    }

    ret = pthread_join(stid, NULL);
    if(ret < 0)
    {
      perror("scheduler thread failed");
      exit(1);
    }

  }


  else if(argc == 5){
    strcpy(alg, argv[2]);
    strcpy(infile, argv[4]);
    pthread_t worktids[n];
    pthread_t stid;
    int i;
    int ret;
    calculate_bCount();
    for(i = 0; i < n; i++){
      fileArray[i] = bCount;
      ret = pthread_create(&worktids[i], NULL, worker_thread_file, (void*) (long) i);
      if(ret < 0 ) {
        perror("thread create failed\n");
        exit(1);
      }
    }

    ret = pthread_create(&stid, NULL, schedule_thread_file, (void*) NULL);
    if(ret < 0 ) {
      perror("thread create failed\n");
      exit(1);
    }

    for(i = 0; i < n; i++){
      pthread_join(worktids[i], NULL);
    }

    ret = pthread_join(stid, NULL);
    if(ret < 0)
    {
      perror("scheduler thread failed");
      exit(1);
    }


  }
  pthread_mutex_destroy(&buf->th_mutex_queue);
  pthread_cond_destroy(&buf->th_cond_hasitem);
  free(buf->q);
  free(buf);
  double result = 0;
  for(int i = 0; i < n; ++i){
    printf("Avarage waiting time thread %d: %lu\n", i,  (avgWaitingTime[i] / fileArray[i]));
    result = result + avgWaitingTime[i] / fileArray[i];
  }
  printf("Avarage of all threads Avarage waiting time is: %f\n", result/n);
  printf("Program ends\n");
  return 0;
}
