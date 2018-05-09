#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

//DATA STRUCT FOR MEMORY SHARING
struct timestruct
{
    unsigned int seconds;
    unsigned int nanoseconds;
};

//DATA STRUCT FOR MESSAGE QUEUE
#define MAXSIZE 4096
typedef struct
{
    long mtype;
    char mtext[MAXSIZE];
    unsigned int timeToRun;
    int running;
    
} mymsg_t;
#define MSG sizeof(mymsg_t)

//PROCESS BLOCK STRUCT
struct processControlBlockStruct
{
    int firstrun;
    int timeLeft;
    unsigned int UnblockTime;
    unsigned int timeUsedLast;
    unsigned int processID;
    unsigned int priority;
    struct timestruct timeInSystem;
};
#define PBLOCKSIZE sizeof(struct processControlBlock)*20

//SHARED MEMORY STRUCT
struct sharedMemoryStruct
{
    struct processControlBlockStruct processBlock[20];
    struct timestruct systemTime;
    unsigned long int blocktime[100000];
    unsigned long int runtime[100000];
    unsigned long int waittime[100000];
};
#define MEMORYSIZE sizeof(struct sharedMemoryStruct)

typedef struct node
{
    int val;
    struct node *next;
} node_t;


//ENQUEUE AND DEQUEUE FUNCTIONS WERE NOT WRITTEN BY ME, I USED AN ONLINE RESOURCE, I DO NOT CLAIM OWNERSHIP.

void enqueue(node_t **head, int val)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) return;

    new_node->val = val;
    new_node->next = *head;

    *head = new_node;
}

int dequeue(node_t **head)
{
    node_t *current, *prev = NULL;
    int retval = -1;

    if (*head == NULL) return -1;

    current = *head;
    while (current->next != NULL)
    {
        prev = current;
        current = current->next;
    }

    retval = current->val;
    free(current);

    if (prev)
        prev->next = NULL;
    else
        *head = NULL;

    return retval;
}

void print_list(node_t *head)
{
    node_t *current = head;

    while (current != NULL)
    {
        printf("Process %d\n", current->val);
        current = current->next;
    }
}


#endif
