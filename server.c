#include "segel.h"
#include "request.h"

#define EMPTY_REQ (-1)

#define SCHEDALG_LEN (7)
#define BLOCK ("block")
#define DT ("dt")
#define DH ("dh")
#define RANDOM ("random")

enum schedalg {block, drop_tail, drop_head, drop_random};
enum req_type {static, dynamic};

typedef struct thread_stats_t{
    int tid;
    int req_count;
    int static_count;
    int dynamic_count;
} *TStats;

typedef struct req_t{
    int fd;
    enum req_type type;
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    TStats handler_thread_stats;
} *Req;

typedef struct queue_t{
    int max;
    int drop;
    int size;  //pending count
    int total_count;
    Req *pendings;
    int tail;
} *Queue;

void req_destroy(Req req){
    close(req->fd);
    if(req->handler_thread_stats)
        free(req->handler_thread_stats);
    free(req);
}

bool q_initialize(Queue q, int max_size);
void q_destroy(Queue q);
void q_push(Queue q, void* val);
Req q_pop(Queue q);
Req q_get(Queue q, int pos);
void q_set(Queue q, int pos, Req req);

Req create_new_request(int connfd);  // TADA!


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

//TADA
void* work(void* ptr) {
    Queue q = (Queue) ptr;
    while (requests == 0) {
        pthread_cond_wait(&pend_and_free, &global_lock);
        pthread_mutex_lock(&global_lock);
        if (requests) {
            '''need to assign job to itself'''
        }

    }

}

static void initialize_threads(int thread_count, pthread_t *workers, Queue q){
    for (i = 0, i < thread_count, i++) {
        pthread_create(&workers[i], NULL, work, q);
    }
}


//with lock
static inline is_overload(Queue q){
    return q->total_count == q->max;
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, queue_size, thread_count;
    enum sched alg;
    struct sockaddr_in clientaddr;

    getargs(argc, argv, &port, &thread_count, &queue_size, &alg);


    int busy_threads = 0;
    int requests = 0;

    //in use for push&pop pending request by main | workers
    pthread_mutex_t global_lock;  //for every access to req_queue and for conditions
    pthread_cond_t pend_and_free;  //availiable pending for workers
    pthread_cond_t queue_not_full;  //availiable slots for new pendings (for 'block' schedlag)

    struct queue_t req_queue;
    q_initialize(&req_queue, queue_size);

    pthread_t *workers = malloc(thread_count * sizeof(pthread_t));
    initialize_threads(thread_count, workers);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        Req new_req = create_new_req(connfd);  //consider moving to inside the global lock
        pthread_mutex_lock(&global_lock);
        if (is_overload(&req_queue)){
            switch (alg)
            {
            case block:
                while(is_overload(&req_queue)){
                    pthread_cond_wait(&queue_not_full, &global_lock);
                    pthread_mutex_lock(&global_lock)
                }
                break;
            case drop_tail:
                break;
            case drop_random:
                q_drop_random(&req_queue);
                break;
            default: //drop_head
                q_del(&req_queue, 0); 
                break;
            }
        }
        
        if(is_overload(&req_queue)) //for (drop_tail) and (drop_random with no pendings) cases
            req_destroy(new_req);

        else{
            q_push(&req_queue, new_req);
            pthread_cond_signal(&pend_and_free);
        }
        pthread_mutex_unlock(&global_lock);
    }
}

//with lock
bool q_initialize(Queue q, int max_size){
    if(max_size <= 0 || (q->pendings = malloc(size(Req) * max_size))==NULL)
        return false;
    q->max = max_size;
    q->drop = ceil((double)max_size * 0.3)
    q->size = 0;
    q->total_count = 0;
    q->tail = 0;
    return true;
}

//with lock
//naive destroy (meanng only frees its value, rather then calling generic given destroy function)
void q_destroy(Queue q){
    for(int pos=0; pos < q->size; ++pos){
        free(q_get(q, pos));
    }
    free(q->pendings);
}

//with lock
void q_push(Queue q, Req req){
    if(q->total_count == q->max)
        return; // consider printing err or returning false
    q_set(q,q->size, req);
    q->size++;
    q->total_count++;
}

//with lock
Req q_pop(Queue q){
    if (q->size == 0)
        return NULL;
    Req req = q_get(q, 0);
    q->tail = (q->tail+1)%(q->max);
    q->size--;
    // we preform the q->total_count-- only after worker finished handling request
    return req;
}

//with lock
Req q_get(Queue q, int pos){
    return q->pendings[(q->tail+pos)%q->max];
}

//with lock
void q_set(Queue q, int pos, Req req){
    q->pendings[(q->tail+pos)%q->max] = req;
}

//with lock
bool q_del(Queue q, int pos){
    if(pos < 0 || pos>q->size)
        return false;
    req_destroy(q_get(q,pos));
    q->size--;
    q->total_count--;
    return true;
}

//with lock
void q_drop_random(Queue q){
    for(int i=0; 0<(q->size) && i<(q->drop); ++i){
        int pos = rand() % q->size;
        q_del(q,pos);
        //updating queue (narrowing gaps)
        for(int j=0; j<pos; ++j){
            q_set(q,pos-j, q_get(q, pos-j-1));
        }
    }
}


 
