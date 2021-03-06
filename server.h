typedef struct thread_stats_t {
    int tid;
    int req_count;
    int static_count;
    int dynamic_count;
} TStats;

typedef struct req_t {
    int fd;
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    TStats handler_thread_stats;
} *Req;

typedef struct queue_t {
    int max;
    int drop;
    int size;  //pending count
    int total_count;
    Req* pendings;
    int tail;
} *Queue;

void req_destroy(Req req);

bool q_initialize(int max_size);

void q_destroy();

void q_push(Req val);

Req q_pop();

Req q_get(int pos);

void q_set(int pos, Req req);

void finish_req(Req req);

Req wait_n_fetch();

void q_drop_random();

bool q_del(int pos);
