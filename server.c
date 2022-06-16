#include "segel.h"
#include "request.h"
#include <assert.h>


#define EMPTY_REQ (-1)

#define SCHEDALG_LEN (7)
#define BLOCK ("block")
#define DT ("dt")
#define DH ("dh")
#define RANDOM ("random")

enum schedalg {block, drop_tail, drop_head, drop_random};

Req create_new_request(int connfd);

void q_drop_random();



///////////// GLOBAL VARIABLES //////////////////

pthread_mutex_t global_lock;  //for every access to req_queue and for conditions

pthread_cond_t pend_and_free;  //availiable pending for workers

pthread_cond_t queue_not_full;  //availiable slots for new pendings (for 'block' schedlag)

struct queue_t req_queue;

Queue q = &req_queue;

//malloc wraper to handle errors
void* Malloc(size_t size){
    void* ptr = malloc(size);
    if(ptr == NULL)
        app_error("malloc failed");
    return ptr;
}

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
        *alg = drop_tail;
    }
    else if(strcmp(argv[4], DH) == 0){
        *alg = drop_head;
    }
    else if(strcmp(argv[4], RANDOM) == 0){
        *alg = drop_random;
    }
    else{
        fprintf(stderr, "<schedalg>: \"block\" | \"dt\" | \"dh\" | \"random\"");
	    exit(1);
    }
}


//with lock
static inline bool is_overload(){
    return q->total_count == q->max;
}

void set_dispatch_time(Req req){
    struct timeval curr;
    int rc = gettimeofday(&curr, NULL);
    assert(rc == 0);
    req->dispatch_interval.tv_sec = curr.tv_sec - req->arrival_time.tv_sec;
    req->dispatch_interval.tv_usec = curr.tv_usec - req->arrival_time.tv_usec;
}

void* work(void* ptr) {
    struct thread_stats_t stats = { *((int*)ptr), 0, 0, 0 };
        while (true) {
            Req req = wait_n_fetch();
            set_dispatch_time(req);
            stats.req_count++;
            requestHandle(req->fd, req, &stats);
            finish_req(req);
        }
}

static void initialize_threads(int thread_count, pthread_t *workers){
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&workers[i], NULL, work, &i);
    }
}
int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, queue_size, thread_count;
    enum schedalg alg;
    struct sockaddr_in clientaddr;

    getargs(argc, argv, &port, &thread_count, &queue_size, &alg);


    int busy_threads = 0;
    int requests = 0;

    q_initialize(queue_size);

    pthread_t *workers = Malloc(thread_count * sizeof(pthread_t));
    initialize_threads(thread_count, workers);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        Req new_req = create_new_request(connfd);  //consider moving to inside the global lock
        pthread_mutex_lock(&global_lock);
        if (is_overload()){
            switch (alg)
            {
            case block:
                while(is_overload()){
                    pthread_cond_wait(&queue_not_full, &global_lock);
                    pthread_mutex_lock(&global_lock)
                }
                break;
            case drop_tail:
                break;
            case drop_random:
                q_drop_random();
                break;
            default: //drop_head (in our impl its "drop queue[tail]")
                q_del(0); 
                break;
            }
        }
        
        if(is_overload()) //for (drop_tail) and (drop_random with no pendings) cases
            req_destroy(new_req);

        else{
            q_push(new_req);
            pthread_cond_signal(&pend_and_free);
        }
        pthread_mutex_unlock(&global_lock);
    }
}

/////////  QUEUE FUNCTIONS IMPLEMENTATION ////////////

//with lock
bool q_initialize(int max_size){
    if(max_size <= 0 || (q->pendings = Malloc(size(Req) * max_size))==NULL)
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
void q_destroy(){
    for(int pos=0; pos < q->size; ++pos){
        free(q_get(q, pos));
    }
    free(q->pendings);
}

//with lock
void q_push(Req req){
    if(q->total_count == q->max)
        return; // consider printing err or returning false
    q_set(q->size, req);
    q->size++;
    q->total_count++;
}

//with lock
Req q_pop(){
    if (q->size == 0)
        return NULL;
    Req req = q_get(0);
    q->tail = (q->tail+1)%(q->max);
    q->size--;
    // we preform the q->total_count-- only after worker finished handling request
    return req;
}

//with lock
Req q_get(int pos){
    return q->pendings[(q->tail+pos)%q->max];
}

//with lock
void q_set(int pos, Req req){
    q->pendings[(q->tail+pos)%q->max] = req;
}

//with lock
bool q_del(int pos){
    if(pos < 0 || pos>q->size)
        return false;
    req_destroy(q_get(q,pos));
    q->size--;
    q->total_count--;
    return true;
}

//with lock
void q_drop_random(){
    for(int i=0; 0<(q->size) && i<(q->drop); ++i){
        int pos = rand() % q->size;
        q_del(pos);
        //updating queue (narrowing gaps)
        for(int j=0; j<pos; ++j){
            q_set(pos-j, q_get(pos-j-1));
        }
    }
}

/////////////// REQUEST FUNCTIONS IMPLEMENTATION ///////////////////


Req create_new_request(int connfd){
   Req req = Malloc(sizeof(struct req_t));
   req->fd = connfd;
   int rc = gettimeofday(&req->arrival_time, NULL)
    assert(rc == 0);
   return req;
}

Req wait_n_fetch() {
    pthread_mutex_lock(&global_lock);
    while (q->size == 0) {
        pthread_cond_wait(&pend_and_free, &global_lock);
        pthread_mutex_lock(&global_lock);
    }
    Req req = q_pop();
    pthread_mutex_unlock(&global_lock);
    return req;
}

void finish_req(Req req) {
    pthread_mutex_lock(&global_lock);
    req_destroy(req);
    q->total_count--;
    pthread_cond_signal(&queue_not_full, &global_lock);
    pthread_mutex_unlock(&global_lock);
}

 
