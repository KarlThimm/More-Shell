/*
CISC361 Thread Project
By: Karl Thimm
*/
typedef void sem_t;
typedef void mnode_t;
typedef void mbox;
void t_create(void(*function)(int), int thread_id, int priority);
void t_yield(void);
void t_init(void);
void t_shutdown(void);
void t_terminate(void);
int sem_init(sem_t **sempoint, unsigned int count);
void sem_signal(sem_t *sempoint);
void sem_wait(sem_t *sempoint);
void sem_destroy(sem_t **sempoint);
int mbox_create(mbox **mb);
void mbox_destroy(mbox **mb);
void mbox_deposit(mbox *mb, char *msg, int len);
void mbox_withdraw(mbox *mb, char *msg, int *len);
void send(int tid, char *msg, int len);
void receive(int *tid, char *msg, int *len);
void block_send(int tid, char *msg, int length);
void block_receive(int *tid, char *msg, int *length);
//Needed function declarations and includes in 2 different files to run test cases