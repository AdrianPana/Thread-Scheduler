/*
 * Threads scheduler header
 *
 * 2017, Operating Systems
 */

#ifndef SO_SCHEDULER_H_
#define SO_SCHEDULER_H_

/* OS dependent stuff */
#ifdef __linux__
#include <pthread.h>

#define DECL_PREFIX

typedef pthread_t tid_t;
#elif defined(_WIN32)
#include <windows.h>

#ifdef DLL_IMPORTS
#define DECL_PREFIX __declspec(dllimport)
#else
#define DECL_PREFIX __declspec(dllexport)
#endif

typedef DWORD tid_t;
#else
#error "Unknown platform"
#endif

 #include <semaphore.h>
 #include <pthread.h>

/*
 * the maximum priority that can be assigned to a thread
 */
#define SO_MAX_PRIO 5
/*
 * the maximum number of events
 */
#define SO_MAX_NUM_EVENTS 256

#define SO_DEFAULT_THREAD_NO 3

#define SO_MAX_UNITS 32

#define THREAD_NEW 0
#define THREAD_READY 1
#define THREAD_RUNNING 2
#define THREAD_BLOCKED 3
#define THREAD_TERMINATED 4

/*
 * return value of failed tasks
 */
#define INVALID_TID ((tid_t)0)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * handler prototype
 */
typedef void (so_handler)(unsigned int);

typedef struct
{   tid_t tid;
    int state;
    int waiting_for_io;
    unsigned int priority;

    so_handler *func;
    pthread_cond_t condition;
    sem_t sem;
} tcb_t;

typedef struct node {
    tcb_t *t;
    struct node *next;
} node_t;

typedef struct {
    node_t *head;
} queue_t;

typedef struct 
{
    unsigned int time_quantum, io, thread_no, thread_cap;
    int io_events[SO_MAX_NUM_EVENTS], current_time_remaining;
    tcb_t *current_thread_running;

    tcb_t **threads;
    queue_t *q;
} scheduler_t;

queue_t *init_queue();
void enqueue(queue_t *q, tcb_t *t);
tcb_t *dequeue(queue_t *q);
void free_queue(queue_t *q);
void print_queue(queue_t *q);

/*
 * creates and initializes scheduler
 * + time quantum for each thread
 * + number of IO devices supported
 * returns: 0 on success or negative on error
 */
DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io);

/*
 * creates a new so_task_t and runs it according to the scheduler
 * + handler function
 * + priority
 * returns: tid of the new task if successful or INVALID_TID
 */
DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority);

/*
 * waits for an IO device
 * + device index
 * returns: -1 if the device does not exist or 0 on success
 */
DECL_PREFIX int so_wait(unsigned int io);

/*
 * signals an IO device
 * + device index
 * return the number of tasks woke or -1 on error
 */
DECL_PREFIX int so_signal(unsigned int io);

/*
 * does whatever operation
 */
DECL_PREFIX void so_exec(void);

/*
 * destroys a scheduler
 */
DECL_PREFIX void so_end(void);

#ifdef __cplusplus
}
#endif

#endif /* SO_SCHEDULER_H_ */

