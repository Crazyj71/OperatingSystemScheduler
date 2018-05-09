//SLAVE PROGRAM
#define _SVID_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/msg.h>
#include <time.h>
#include "sharedmemory.h"

//GLOBALS
int k;
int run;
int shm_id;
int msqid;


//SIGNAL HANDLER
void HANDLER(int i)
{
    printf("PROCESS %d Died in Action\n", k);

  msgctl(msqid, IPC_RMID, NULL);          //FREE SHARED MESSAGE QUEUE
        shmctl(shm_id, IPC_RMID, NULL);         //FREE SHARED MEMORY DATA

    exit(0);

    run=1;

}

////////////////////////////////////////MAIN FUNCTION
int main(int argc, char *argv[])
{

    printf("SLAVE EXECUTED\n");

//DEFINE GLOBAL VALUES
    run=0;
    k = atoi(argv[1]);

    char filename[20];
    memcpy(filename,(argv[3]), 19);

//SET UP SIGNAL HANDLING
    while(run==0)
    {
        signal(SIGINT, HANDLER);

//SHARED MEMORY VARIABLES
        key_t           mem_key;
        struct sharedMemoryStruct     *mem;

//PREPARE IDENTIFIERS
        mem_key = ftok("a", 1);
        shm_id = shmget(mem_key, sizeof(struct sharedMemoryStruct), 0666);
        if (shm_id < 0)
        {
            perror("shmget");
            exit(1);
        }

//ATTACH TO DATA STRUCT
        mem = (struct sharedMemoryStruct *) shmat(shm_id, NULL, 0);
        if ((long) mem == -1)
        {
            perror("shmat");
            exit(1);
        }

//INITIALIZE MESSAGE QUEUE FOR CRITICALS
        key_t key; /* key to be passed to msgget() */
//int msgflg; /* msgflg to be passed to msgget() */
        mymsg_t *message;
        message = (mymsg_t *)malloc(sizeof(mymsg_t));
        mymsg_t *message2;
        message2 = (mymsg_t *)malloc(sizeof(mymsg_t));
	key=ftok("/b",1);		//MAKE A KEY
        message->mtype=k+1;
        message2->mtype=k+21;
	msqid = msgget(key, 0666);	//CREATE ID

//MESSAGE QUEUE SUCCESS
        printf("SLAVE ATTACHED TO MESSAGE QUEUE 1\n");
	
	unsigned long int starttime= mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds;
	unsigned long int waittime= mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds;
	unsigned long int blocktime= mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds;
	int blockflag=0;	

	srand(time(NULL)*k);
	int terminateThreshold = 10;
	while(1)
	{
	fflush(stdout);
	int terminate = rand()%1000 + 1;
	msgrcv(msqid, message, MSG, k+1, 0);

	waittime=(mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds) - waittime;
	for(int t=0; t<100000; t++){
        if(mem->waittime[t]==0){
        mem->waittime[t]= waittime; break;}}
	waittime= mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds;

	if(blockflag==1){blockflag=0;
	blocktime=(mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds) - blocktime;
	for(int t=0; t<100000; t++){
        if(mem->blocktime[t]==0){
        mem->blocktime[t]= blocktime; break;}
	}
	}

        printf("Context Switched to Process %d\n", k);
	if(terminate<=terminateThreshold || message->running==0)
	{//TERMINATED
	int timeRan = rand()%(message->timeToRun)+100;
	message2->timeToRun = timeRan;
	message2->running = 0;
	msgsnd(msqid, message2, MSG, IPC_NOWAIT);
	if(message->running==0)
	printf("Process %d Terminated by Operating System\n", k);
	else printf("Process %d Terminated on its own\n", k);

	unsigned long int endtime = (mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds) - starttime;
	
	for(int t=0; t<100000; t++){
	if(mem->runtime[t]==0){
	mem->runtime[t]= endtime; break;}

	}
	return 0;
	}
	else
	{
	int BlockedThreshold=5;
	int Blocked = rand()%100+1;
	if(Blocked<BlockedThreshold){//Do blocked Stuff
	mem->processBlock[k].UnblockTime += mem->systemTime.nanoseconds + rand()%1000000;
	int timeRan = rand()%(message->timeToRun)+100;
	message2->timeToRun = timeRan;
	message2->running = 2;//blocked code

	blockflag=1;
	blocktime= mem->systemTime.seconds*1000000000 + mem->systemTime.nanoseconds;

	msgsnd(msqid, message2, MSG, IPC_NOWAIT);
	

	}else{//Ran Normally
	message2->timeToRun = message->timeToRun;
	message2->running = 1;
	printf("Process %d ran as scheduled\n",k);
	msgsnd(msqid, message2, MSG, IPC_NOWAIT);
	}
	}
	
	}//while(1)
    }//whilerun
    return 0;
}//main
