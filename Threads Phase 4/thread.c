/*
CISC361 Thread Project
By: Karl Thimm
*/

#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include "thread.h"

tcb_t *running = NULL;
tcb_t *waiting = NULL;
tcb_t *waiting2 = NULL;
tcb_2_t *thread_list = NULL;

/*This function allows a current thread to yield to another thread if
there is one available. If not, the current thread continues to execute.
This allows multiple threads to execute.*/
void t_yield () {
    if (!waiting) {
        setcontext(running->thread_context);
    }

    waiting2->next = running;
    waiting2 = waiting2->next;

    running = waiting;
    waiting = waiting->next;
    running->next = NULL;
    swapcontext(waiting2->thread_context, running->thread_context);
}

/* this function sets up the initial state of the thread management system 
by creating a new thread control block for the main thread and initializing 
the necessary data structures.*/
void t_init () {

    ucontext_t *tmp;
    tmp = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(tmp);    /* let tmp be the context of main() */

    mbox *mb;
    mbox_create(&mb);

    tcb_t *main = malloc(sizeof(tcb_t));
    main->thread_id = 0;
    main->thread_priority = 1;
    main->thread_context = tmp;
    main->mb = mb;
    main->next = NULL;

    tcb_2_t *node = malloc(sizeof(tcb_2_t));
    node->tcb = main;
    node->next = NULL;
    thread_list = node;

    running = main;

}

/*This function creates a new thread with the provided function, ID, and priority, 
and adds it to the waiting queue for later execution.*/
int t_create (void (*fct)(int), int id, int pri) {

    size_t sz = 0x10000;

    ucontext_t *uc;
    uc = (ucontext_t *) malloc(sizeof(ucontext_t));

    getcontext(uc);

    uc->uc_stack.ss_sp = malloc(sz);  /* new statement */
    uc->uc_stack.ss_size = sz;
    uc->uc_stack.ss_flags = 0;
    uc->uc_link = running->thread_context; 
    makecontext(uc, (void (*)(void)) fct, 1, id);

    mbox *mb;
    mbox_create(&mb);

    tcb_t *control_block = malloc(sizeof(tcb_t));
    control_block->thread_id = id;
    control_block->thread_priority = pri;
    control_block->thread_context = uc;
    control_block->mb = mb;
    control_block->next = NULL;

    tcb_2_t *node = malloc(sizeof(tcb_2_t));
    node->tcb = control_block;
    node->next = NULL;
    
    tcb_2_t *temp = thread_list;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = node;

    if (!waiting) {
        waiting = control_block;
        waiting2 = waiting;
    } else {
        waiting2->next = control_block;
        waiting2 = waiting2->next;
    }
}

/*This function terminates the current thread and switches to the next thread in the waiting 
queue, freeing any memory allocated for the current thread at the same time.*/
void t_terminate () {

    tcb_t *temp = running;

    running = waiting;
    waiting = waiting->next;
    running->next = NULL;
    free(temp->thread_context->uc_stack.ss_sp);
    free(temp->thread_context);
    mbox_destroy(&(temp->mb));
    free(temp);
    setcontext(running->thread_context);
}

/*function is used to shut down the thread system and free any memory allocated during the thread execution. 
It frees the memory of all waiting threads and then frees the memory of the thread 
that was currently current al well as the current pointer itself*/
void t_shutdown () {

    free(running->thread_context->uc_stack.ss_sp);
    free(running->thread_context);
    mbox_destroy(&(running->mb));
    free(running);

    while (waiting) {
        tcb_t *temp = waiting->next;
        free(waiting->thread_context->uc_stack.ss_sp);
        free(waiting->thread_context);
        mbox_destroy(&(waiting->mb));
        free(waiting);
        waiting = temp;
    }

    while (thread_list) {
        tcb_2_t *temp = thread_list->next;
        free(thread_list);
        thread_list = temp;
    }
}

/*This is a function that initializes a semaphore pointed to by **sempoint with an 
initial count value of "count".
The function first allocates memory for a sem_t struct using the malloc 
function and assigns the address to *sempoint.
It then sets the initial count value and initializes the queue to NULL.
Finally, it returns 0 to indicate that the initialization was successful.*/
int sem_init(sem_t **semaphore, unsigned int count) {
    
    *semaphore = (sem_t *) malloc(sizeof(sem_t));
    (*semaphore)->count = count;
    (*semaphore)->queue = NULL;

    return 0;
}

/*This is a function that decrements a semaphore and puts the calling thread to 
sleep if there are no resources available. It blocks a signal to avoid race conditions, 
adds the thread to the semaphore queue if needed, and switches to the next waiting 
thread. Once a resource is available, it wakes up the waiting thread and unblocks 
the signal.*/
void sem_wait(sem_t *semaphore) {
    sighold(SIGALRM);
    semaphore->count--;
    if (semaphore->count < 0) {
        if (!waiting) {
            setcontext(running->thread_context);
        }
        if (semaphore->queue) {
            tcb_t *temp = semaphore->queue;
            semaphore->queue = running;
            semaphore->queue->next = temp;
        } else {
            semaphore->queue = running;
        }
        running = waiting;
        waiting = waiting->next;
        running->next = NULL;

        swapcontext(semaphore->queue->thread_context, running->thread_context);
    }

    sigrelse(SIGALRM);
}

/*This is a function that increments the value of a semaphore pointed to by "sempoint". 
If there are threads waiting on the semaphore, it wakes up the first waiting thread.
The function first blocks the SIGALRM signal to avoid race conditions, 
and then increments the semaphore count.
If there are threads waiting on the semaphore, the function removes the 
first waiting thread from the queue by updating the "queue" pointer of the semaphore struct.
The function then updates the "next" pointer of the last waiting thread to 
point to the removed thread, effectively adding it to the waiting list.
The function ends by calling sigrelse to unblock the SIGALRM signal.*/
void sem_signal(sem_t *semaphore) {
    sighold(SIGALRM);

    semaphore->count++;

    if (semaphore->queue) {

        tcb_t *tail_prev = NULL;
        tcb_t *tail = semaphore->queue;
        while (tail->next) {
            tail_prev = tail;
            tail = tail->next;
        }
        if (tail_prev) {
            tail_prev->next = NULL;
        } else {
            semaphore->queue = NULL;
        }

        if (waiting) {
            waiting2->next = tail;
            waiting2 = waiting2->next;
        } else {
            waiting = tail;
            waiting2 = tail;
        }
    }

    sigrelse(SIGALRM);
}

/*This function frees the memory allocated for a semaphore and its waiting 
threads by iterating through the queue and freeing each thread's context, stack, 
and TCB structs. It then frees the memory allocated for the semaphore struct.*/
void sem_destroy(sem_t **semaphore) {
    while ((*semaphore)->queue) {
        tcb_t *temp = (*semaphore)->queue->next;
        free((*semaphore)->queue->thread_context->uc_stack.ss_sp);
        free((*semaphore)->queue->thread_context);
        free((*semaphore)->queue);
        (*semaphore)->queue = temp;
    }

    free((*semaphore));
}


int mbox_create(mbox **mb) {

    sem_t *semaphore;
    sem_init(&semaphore, -1);

    *mb = (mbox *) malloc(sizeof(mbox));
    (*mb)->mnode = NULL;
    (*mb)->sem = semaphore;

    return 0;
}

void mbox_destroy(mbox **mb) {

    mnode_t *message_node = (*mb)->mnode;

    while (message_node) {
        mnode_t *temp = message_node->next;
        free(message_node->msg);
        free(message_node);
        message_node = temp;
    }

    sem_destroy(&((*mb)->sem));

    free((*mb));
}

void mbox_deposit(mbox *mb, char *msg, int len) {

    char *message = malloc(sizeof(char) * (len + 1));
    strcpy(message, msg);

    mnode_t *message_node = malloc(sizeof(mnode_t));
    message_node->msg = message;
    message_node->len = len;
    message_node->sender = -1;
    message_node->receiver = -1;

    if (mb->mnode) {
        message_node->next = mb->mnode;
    } else {
        message_node->next = NULL;
    }

    mb->mnode = message_node;

}

void mbox_withdraw(mbox *mb, char *msg, int *len) {

    if (!mb->mnode) {
        *len = 0;
        return;
    }

    mnode_t *previous = NULL;
    mnode_t *message_node = mb->mnode;

    while (message_node->next) {
        previous = message_node;
        message_node = message_node->next;
    }

    strcpy(msg, message_node->msg);
    *len = message_node->len;

    if (previous) {
        previous->next = NULL;
    } else {
        mb->mnode = NULL;
    }

    free(message_node->msg);
    free(message_node);
}

void send(int tid, char *msg, int len) {

    int sender_id = running->thread_id;
    int receiver_id = tid;

    tcb_2_t *rec_thread_n = thread_list;
    tcb_t *rec_thread = NULL;

    while (rec_thread_n) {
        if (rec_thread_n->tcb->thread_id == receiver_id) {
            rec_thread = rec_thread_n->tcb;
            break; 
        } else {
            rec_thread_n = rec_thread_n->next;
        }
    }

    mnode_t *message_node = malloc(sizeof(mnode_t));
    message_node->msg = malloc(sizeof(char) * (len + 1));
    strcpy(message_node->msg, msg);
    message_node->len = len;
    message_node->sender = sender_id;
    message_node->receiver = receiver_id;
    
    if (rec_thread->mb->mnode) {
        message_node->next = rec_thread->mb->mnode;
    } else {
        message_node->next = NULL;
    }

    rec_thread->mb->mnode = message_node;

    sem_signal(rec_thread->mb->sem);
}

void receive(int *tid, char *msg, int *len) {

    int sender_id = *tid;
    int receiver_id = running->thread_id;

    mnode_t *previous = NULL;
    mnode_t *current = running->mb->mnode;
    mnode_t *found = NULL;

    if (!current) {
        sem_wait(running->mb->sem);
    }

    if (sender_id == 0) {
        while (current->next) {
            previous = current;
            current = current->next;
        }
        found = current;
    } else {
        while (current) {
            if (current->sender == sender_id) { 
                found = current;
                break;
            }
            previous = current;
            current = current->next;
        }
    }

    if (found) {
        *tid = found->sender;
        strcpy(msg, found->msg);
        *len = found->len;

        if (previous) {
            previous->next = found->next;
        } else {
            running->mb->mnode = NULL;
        }

        running->mb->sem->count--;

        free(found->msg);
        free(found);
    } else {
        *len = 0;
    }
}

void block_send(int tid, char *msg, int length) {
    int sender_id = running->thread_id;
    int receiver_id = tid;

    tcb_2_t *rec_thread_n = thread_list;
    tcb_t *rec_thread = NULL;

    while (rec_thread_n) {
        if (rec_thread_n->tcb->thread_id == receiver_id) {
            rec_thread = rec_thread_n->tcb;
            break; 
        } else {
            rec_thread_n = rec_thread_n->next;
        }
    }
    mnode_t *message_node = malloc(sizeof(mnode_t));
    message_node->msg = malloc(sizeof(char) * (length + 1));
    strcpy(message_node->msg, msg);
    message_node->len = length;
    message_node->sender = sender_id;
    message_node->receiver = receiver_id;
    
    if (rec_thread->mb->mnode) {
        message_node->next = rec_thread->mb->mnode;
    } else {
        message_node->next = NULL;
    }

    rec_thread->mb->mnode = message_node;

    sem_signal(rec_thread->mb->sem);

    mnode_t *node = rec_thread->mb->mnode;

    t_yield();
}

void block_receive(int *tid, char *msg, int *length) {

    int sender_id = *tid;
    int receiver_id = running->thread_id;

    mnode_t *previous = NULL;
    mnode_t *current = running->mb->mnode;
    mnode_t *found = NULL;
    if (!current) {
        sem_wait(running->mb->sem);
        current = running->mb->mnode;
    }
    if (sender_id == 0) {
        while (current->next) {
            previous = current;
            current = current->next;
        }
        found = current;
    } else {
        while (current) {
            if (current->sender == sender_id) { 
                found = current;
                break;
            }
            previous = current;
            current = current->next;
        }
    }
    if (found) {
        *tid = found->sender;
        strcpy(msg, found->msg);
        *length = found->len;

        if (previous) {
            previous->next = found->next;
        } else {
            running->mb->mnode = NULL;
        }
        running->mb->sem->count--;

        free(found->msg);
        free(found);
    } else {
        *length = 0;
    }

}