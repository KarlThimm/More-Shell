#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>

typedef struct mailBox mbox;
typedef struct tcb tcb_t;
typedef struct semaphore sem_t;
typedef struct tcb_linked_list_node tcb_2_t;

struct tcb {
	int thread_id;
  int thread_priority;
	ucontext_t *thread_context;
	struct mailBox *mb;
	struct tcb *next;
};

struct tcb_linked_list_node {
	tcb_t *tcb;
	struct tcb_linked_list_node *next;
};

struct semaphore {
	int count;
	tcb_t *queue;
};

struct messageNode {
	char *msg;
	int len;
	int sender;
	int receiver;
	struct messageNode	*next;
};

typedef struct messageNode mnode_t;

struct mailBox {
	mnode_t *mnode;
	sem_t   *sem;
};