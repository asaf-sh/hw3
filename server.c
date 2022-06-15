#include "segel.h"
#include "request.h"

#define EMPTY_REQ (-1)

#define SCHEDALG_LEN (7)
#define BLOCK ("block")
#define DT ("dt")
#define DH ("dh")
#define RANDOM ("random")

enum schedalg {block, dt, dh, random};
// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
typedef struct thread_stats_t{
    int tid;
    int req_count;
    int static_count;
    int dynamic_count;
} *TStats;

typedef struct req_t{
    int fd;
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    TStats handler_thread_stats;
} *Req;

typedef struct queue_t{
    int max;
    int size;
    void **arr;
    int head,tail;
} *Queue;

bool q_initialize(Queue q, int max_size);
void q_destroy(Queue q);
void q_push(Queue q, void* val);
void* q_pop(Queue q);


void getargs(int argc, char *argv[], int *port, int* thread_count, int *queue_size, enum schedalg *alg)
{
    if (argc != 4) {
	    fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
	    exit(1);
    }
    *port = atoi(argv[1]);
    *thread_count = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    
    if(strcmp(argv[4], BLOCK) == 0){
        *alg = block;
    }
    else if(strcmp(argv[4], DT) == 0){
        *alg = dt;
    }
    else if(strcmp(argv[4], DH) == 0){
        *alg = dh;
    }
    else if(strcmp(argv[4], RANDOM) == 0){
        *alg = random;
    }
    else{
        fprintf(stderr, "<schedalg>: \"block\" | \"dt\" | \"dh\" | \"random\"");
	    exit(1);
    }
}

void* work(void* ptr) {
    while (requests == 0) {
        pthread_cond_wait(&pend_and_free, &global_lock);
        pthread_mutex_lock(&global_lock);
        if (requests) {
            '''need to assign job to itself'''
        }

    }

}

bool addRequest(int fd, int pendings_size, int** pendings) {
    int i = 0;
    while (&pendings[i] == -1 && i < pendings_size) {
        i++;
    }
    if (i < pendings_size) {
        &pendings[i] = fd;
        return true;
    }
    else {
        return false;
    }
}

static void initialize_threads(int thread_count, pthread_t *workers){
    for (i = 0, i < thread_count, i++) {
        pthread_create(&workers[i], NULL, work, NULL);
    }
}
static void initialize_pendings(int max_requests, int *pendings){
    for (i = 0, i < max_requests, i++) {
        pendings[i] = EMPTY_REQ;
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    int pendings_size = argv[3];
    int thread_count = args[2];

    int busy_threads = 0;
    int requests = 0;

    pthread_mutex_t global_lock;
    pthread_cond_t pend_and_free;


    int *pendings = malloc(pendings_size * sizeof(int));
    pthread_t *workers = malloc(thread_count * sizeof(pthread_t));

    initialize_pendings(pendings_size, pendings);
    initialize_threads(thread_count, workers);
    

    // 
    // HW3: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
    pthread_mutex_lock(&global_lock);
    accept_to_pend = addRequest(connfd, *pendings);
    if (!accept_to_pend) {
        handleOverload();
    }
    pthread_cond_signal(&pend_and_free);


	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}

bool q_initialize(Queue q, int max_size){
    if(max_size <= 0 || (q->arr = malloc(size(void*) * max_size))==NULL)
        return false;
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    return true;
}
//naive destroy (meanng only frees its value, rather then calling generic given destroy function)
void q_destroy(Queue q){
    for(int i=q->tail; q->size > 0; i=(i+1)%(q->max)){
        free(q->arr[i]);
        q->size--;
    }
    free(q->arr);
}

void q_push(Queue q, void* val){
    q->arr[q->head] = val;
    q->head = (q->head)%(q->max);
    q->size++;
}

void* q_pop(Queue q){
    if (q->size == 0)
        return NULL;
    void* val = q->arr[q->tail];
    q->tail = (q->tail+1)%(q->max);
    q->size--;
    return val;
}

    


 
