#include <stdlib.h>
#include "so_scheduler.h"
#include <pthread.h>
#include <semaphore.h>

static scheduler_t *s;
static int scheduler_init = 0;

// Se scoate din coada urmatorul thread si se planifica pe procesor
// Thread-ul precedent se blocheaza
void schedule_next_thread()
{
    tcb_t *thread = dequeue(s->q);

    if (thread != NULL) {
        s->current_thread_running = thread;
        s->current_thread_running->state = THREAD_RUNNING;
        s->current_time_remaining = s->time_quantum;

        sem_post(&s->current_thread_running->sem);
    }
}

// Se verifica thread-ul curent in raport cu planificatorul si,
// daca e cazul, se preempteaza si se planifica alt thread
void check_scheduler()
{
    tcb_t *current_thread_running = s->current_thread_running;

    if (current_thread_running == NULL)
        {
            schedule_next_thread();
            return;
        }

    // Thread-ul asteapta sau s-a incheiat
    if (current_thread_running->state != THREAD_RUNNING)
        {
            schedule_next_thread();
            return;
        }

    if (s->current_time_remaining <= 0) {
        // Daca i s-a terminat timpul, dar nu are cu cine fi inlocuit,
        // se reseteaza cuanta de timp
        if (s->q->head == NULL || s->q->head->t->priority < current_thread_running->priority) {
            s->current_time_remaining = s->time_quantum;
            sem_post(&current_thread_running->sem);
            return;
        }

        // Altfel, este inlocuit
        else {
            current_thread_running->state = THREAD_READY;
            enqueue(s->q, current_thread_running);
            schedule_next_thread();
            return;
        }
    }

    // Daca nu exista thread-uri in asteptare, isi continua executia
    if (s->q->head == NULL) {
        sem_post(&current_thread_running->sem); 
        return;
    }

    // Daca exista thread-uri READY cu prioritate mai mare, se preempteaza si
    // se reintroduce in coada de prioritati
    if (current_thread_running->priority < s->q->head->t->priority) {
        current_thread_running->state = THREAD_READY;
        enqueue(s->q, current_thread_running);
        schedule_next_thread();
        return;
    }

    sem_post(&current_thread_running->sem);
    return;

}

// Se pregateste thread-ul pentru executie
DECL_PREFIX void start_thread(void *arg) 
{
    tcb_t *thread = (tcb_t *)arg;
    
    sem_wait(&thread->sem);

    thread->func(thread->priority);

    thread->state = THREAD_TERMINATED;

    check_scheduler();

    return;
}

DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io) 
{
    // Verifica cuanta de timp
    if (time_quantum == 0)
        return -1;

    // Verifica numarul de evenimente
    if (io > SO_MAX_NUM_EVENTS)
        return -1;

    // Verifica reinitializarea
    if (scheduler_init == 1)
        return -1;

    // Se aloca memorie pentru planificator, array-ul de thread-uri si
    // pentru coada de prioritati
    scheduler_init = 1;
    s = malloc(sizeof(scheduler_t));
    if (s == NULL)
        return -1;

    s->time_quantum = time_quantum;
    s->io = io;
    s->current_thread_running = NULL;
    s->current_time_remaining = 0;
    s->thread_no = 0;
    s->thread_cap = SO_DEFAULT_THREAD_NO;

    s->threads = malloc(s->thread_cap * sizeof(tcb_t *));
    if(s->threads == NULL) {
        free(s);
        return -1;
    }

    s->q = init_queue();

    return 0;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
    // Se verifica handler-ul
    if (func == 0)
        return INVALID_TID;
    
    // Se verifica prioritatea
    if (priority > SO_MAX_PRIO)
        return INVALID_TID;

    // Se aloca mai multa memorie pentru thread-uri daca s-a ajuns la final
    s->thread_no++;
    if (s->thread_no >= s->thread_cap) {
        s->thread_cap *= 2;
        s->threads = realloc(s->threads, s->thread_cap * sizeof(tcb_t *));

        if(s->threads == NULL)
        return INVALID_TID;
    }

    // Se initializeaza noul thread
    s->threads[s->thread_no - 1] = malloc(sizeof(tcb_t));
    tcb_t *new_thread = s->threads[s->thread_no - 1];
    new_thread->tid = INVALID_TID;
    new_thread->state = THREAD_READY;
    new_thread->priority = priority;
    new_thread->waiting_for_io = -1;
    new_thread->func = func;
    sem_init(&(new_thread->sem), 0, 0);

    pthread_create(&new_thread->tid, NULL, (void *)start_thread, (void *)new_thread);

    // Se adauga thread-ul in coada de prioritati
    enqueue(s->q, new_thread);

    // Daca este primul fork() apelat, se va planifica noul thread
    if (s->current_thread_running == NULL)
    {
        schedule_next_thread();
    }
    // Altfel, se scade timpul si se verifica planificatorul
    else {
        so_exec();
    }

    return new_thread->tid;
}

DECL_PREFIX int so_wait(unsigned int io)
{
    // Se verifica evenimentul 
    if (io >= s->io || io < 0)
        return -1;

    // Se modifica thread-ul ca fiind blocat si in asteptarea evenimentului
    tcb_t *current_thread_running = s->current_thread_running;
    current_thread_running->state = THREAD_BLOCKED;
    current_thread_running->waiting_for_io = io;

    // Se scade timpul si se verifica planificatorul
    so_exec();

    return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
    // Se verifica evenimentul 
    if (io >= s->io || io < 0)
        return -1;

    int unblocked_threads = 0; 
    
    // Se trece prin toate thread-urile si se "deblocheaza" cele care asteapta
    // pentru evenimentul respectiv, se marcheaza READY si se adauga in coada
    for (int i = 0; i < s->thread_no; i++) {

        if (s->threads[i]->state == THREAD_BLOCKED && s->threads[i]->waiting_for_io == io) {
            unblocked_threads++;

            s->threads[i]->state = THREAD_READY;
            s->threads[i]->waiting_for_io = -1;
            enqueue(s->q, s->threads[i]);
        }
    }

    // Se scade timpul si se verifica planificatorul
    so_exec();

    return unblocked_threads;
}

DECL_PREFIX void so_exec(void)
{
    // Se scade timpul si se verifica planificatorul
    s->current_time_remaining--;

    tcb_t *current_thread_running = s->current_thread_running;

    check_scheduler();

    // Se blocheaza thread-ul respectiv in cazul in care este preemptat
    sem_wait(&current_thread_running->sem);

    return;
}

DECL_PREFIX void so_end(void)
{
    // Se verifica daca planificatorul a fost initializat
    if (!scheduler_init)
        return;

    // Se asteapta ca toate thread-urile sa isi termine executia
    for (int i = 0; i < s->thread_no; i++)
    {
        pthread_join(s->threads[i]->tid, NULL);
        sem_destroy(&s->threads[i]->sem);
    }

    // Se elibereaza memoria planificatorului
    for (int i = 0; i < s->thread_no; i++)
        free(s->threads[i]);
    free(s->threads);

    free_queue(s->q);
    
    scheduler_init = 0;
    free(s);
}

// Functiile obisnuite pentru o coada pentru prioritati
node_t *init_node(tcb_t *t) 
{
    node_t *node = malloc(sizeof(node_t));

    node->t = t;
    node->next = NULL;

    return node;
}

void free_node(node_t *node)
{
    free(node);
    return;
}

queue_t *init_queue()
{
    queue_t *q = malloc(sizeof(queue_t));
    q->head = NULL;

    return q;
}

void enqueue(queue_t *q, tcb_t *t)
{
    node_t *new_node = init_node(t);

    node_t *node = q->head;

    if (node == NULL)
    {
        q->head = new_node;
        return;
    }

    if (new_node->t->priority > node->t->priority)
    {
        new_node->next = node;
        q->head = new_node;
        return;
    }

    if (node->next == NULL)
    {
        node->next = new_node;
        return;
    }

    node_t *prev = node;

    while (node != NULL && new_node->t->priority <= node->t->priority) {
        prev = node;
        node = node->next;
    }

    new_node->next = node;
    prev->next = new_node;
    
    return;
}

tcb_t *dequeue(queue_t *q)
{
    if (q->head == NULL)
        return NULL;

    node_t *node = q->head;
    q->head = q->head->next;

    tcb_t *thread = node->t;
    free(node);

    return thread;
}

void free_queue(queue_t *q)
{
    if (q->head != NULL)
    {
        node_t *node = q->head;

        while(node->next != NULL)
        {
            node_t *new_node = node->next;
            free(node);
            node = new_node;
        }
        free(node);
    }

    free(q);
    return;
}