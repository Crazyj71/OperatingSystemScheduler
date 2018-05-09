//MASTER PROGRAM
#define _GNU_SOURCE
#define _SVID_SOURCE
#define _POSIX_SOURCE
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
#include <time.h>
#include <sys/msg.h>
#include "sharedmemory.h"

//GLOBALS
int pid;
int run;
int shm_id;
int msqid;
int i = -1;

//KILL CHILDREN
void kill_child(int sig)
{
    printf("SIMULATION TIME OUT\n %d Processes Ran\n", i);
    while(1)
    {
        int childpid=wait(NULL);
        if(childpid==-1) break;
    }

    if(pid!=getpid())
        kill(getpid(),SIGINT);

    msgctl(msqid, IPC_RMID, NULL);          //FREE SHARED MESSAGE QUEUE
    shmctl(shm_id, IPC_RMID, NULL);         //FREE SHARED MEMORY DATA

    exit(0);
}

void HANDLER(int sig)
{
    while(1)
    {
        int childpid=wait(NULL);
        if(childpid==-1) break;
    }
    msgctl(msqid, IPC_RMID, NULL);          //FREE SHARED MESSAGE QUEUE
    shmctl(shm_id, IPC_RMID, NULL);         //FREE SHARED MEMORY DATA
    exit(0);
}

//LOG FILE PRINT FUNCTIONS//////////////////////////////////////////////////////
char filename[20];
void writeLogNewProcess(FILE *fp, int PID, int QUEUE, int sec, int nano)
{
    fp=fopen(filename,"a");
    fprintf(fp, "CREATE: Generating process with PID %d and putting it in queue %d at time %d:%d\n", PID, QUEUE, sec, nano);
    fclose(fp);
}/////////////////////////////////////////////////////////////////////////////////////////////////////

void writeLogRunProcess(FILE *fp, int PID, int QUEUE, int sec, int nano, int dispatch)
{
    fp=fopen(filename,"a");
    fprintf(fp, "RUN:    Dispatching process with PID %d from queue %d at time %d:%d, this dispatch was %d nanoseconds\n", PID,
            QUEUE,sec,nano,dispatch);
    fclose(fp);
}/////////////////////////////////////////////////////////////////////////////////////////////////////

void writeLogQueue(FILE *fp, int PID, int QUEUE)
{
    fp=fopen(filename,"a");
    fprintf(fp, "QUEUE:  Putting Process %d into Queue %d", PID, QUEUE);
    if(QUEUE==4)fprintf(fp, "(BLOCKED)");
    fprintf(fp, "\n");
    fclose(fp);
}/////////////////////////////////////////////////////////////////////////////////////////////////////

void writeLogEnd(FILE *fp, int PID, int sec, int nanosec)
{
    fp=fopen(filename,"a");
    fprintf(fp, "END:    Process %d ended after running for %d:%d\n", PID, sec, nanosec);
    fclose(fp);
}/////////////////////////////////////////////////////////////////////////////////////////////////////

void writeLogUnblock(FILE *fp, int PID)
{
    fp=fopen(filename,"a");
    fprintf(fp, "UNBLK:  Process %d removed from the blocked queue(UNBLOCKED)\n",PID);
    fclose(fp);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////MAIN//////////////////////////////////////////
int main(int argc, char *argv[])
{
//SET GLOBALS
    run=0;
    pid=getpid();

//SET UP SIGNAL HANDLING
    signal(SIGALRM,(void (*)(int))kill_child);
    signal(SIGINT, HANDLER);
    while(run==0)
    {

//INITIALIZE VARS
        char opt;
        int s;
        int z;
        pid_t childpid;

//GET OPTION IS SET
        while ( (opt= getopt(argc,argv, "dhs:l:t:")) !=-1 )
        {
            switch ( opt)
            {
            case 'h':
                printf("-s [slave#]\n -l [logfile]\n -t [time]\n -d FOR DEFAULTS");
                return 0;
            case 'l':
                memcpy(filename, optarg, 20);
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                z = atoi(optarg);
                break;
            case 'd':
                s = 1;
                z=20;
                memcpy(filename, "file.log", 10);
                break;
            }
        }

//CHECK FOR VALID NUMBER OF COMMAND ARGS & VALID N
        if (argc == 1 )
        {
            fprintf(stderr, "See ./oss -h for help\n");
            return 1;
        }


//INITIALIZE BIT-ARRAY
        int processOpen[20];
        int pidArray[20];
        for(int j=0; j<20; j++)
        {
            processOpen[j] = 0;
            pidArray[20]=0;
        }






//INITIALIZE TIME BETWEEN PROCESSES
        int maxTimeBetweenNewProcs = 1000000000;

//INITIALIZE REALTIME PROCESS FRACTION
        float realtime= .2;

//INITIALIZE QUEUES
        node_t *priority0 = NULL;
        node_t *priority1 = NULL;
        node_t *priority2 = NULL;
        node_t *priority3 = NULL;
        node_t *BlockQueue= NULL;

//SET AN ALARM IF PROGRAM TAKES TOO LONG
        alarm(z);

//SHOW USER WHAT ARGS WE RECEIVED
        printf("Running with S:%d T:%d L:%s\n", s, z, filename);

//INITIALIZE DATA VARS
        key_t     mem_key;
        struct sharedMemoryStruct *shm_ptr;

//GENERATE KEY
        mem_key = ftok("a", 1);
        shm_id = shmget(mem_key, MEMORYSIZE, IPC_CREAT |
                        0666);
        if (shm_id < 0)
        {
            perror("shmget");
            exit(1);
        }

//ATTACH OR CREATE DATA STRUCT
        shm_ptr = (struct sharedMemoryStruct *) shmat(shm_id, NULL, 0);
        /*
        attach */
        if ((long) shm_ptr == -1)
        {
            perror("shmat");
            exit(1);
        }

//Initialize DATA
        shm_ptr->systemTime.seconds=0;
        shm_ptr->systemTime.nanoseconds=0;
	
	for(int j=0; j<100000; j++){
	shm_ptr->blocktime[j] = 0;
        shm_ptr->runtime[j] = 0;
        shm_ptr->waittime[j] = 0;
	}
//DATA STRUCT SUCCESS
        printf("data struct made\n");

//INITIALIZE MESSAGE QUEUE
        key_t key; /* key to be passed to msgget() */
//int msgflg; /* msgflg to be passed to msgget() */

//INITIALIZE PROCESS MESSAGE QUEUES
        mymsg_t *message[40];
        for(int j=0; j<40; j++)
        {
            message[j] = (mymsg_t *) malloc(sizeof(mymsg_t));
            message[j]->mtype=j+1;
            memcpy(message[j]->mtext, "", 100);
        }

        key=ftok("/b",1);					//GENERATE KEY
        msqid = msgget(key, (IPC_CREAT | 0666));
        msgctl(msqid, IPC_RMID, NULL);
        msqid = msgget(key, (IPC_CREAT | 0666));		//IF MSQID EXISTS, REMOVE IT AND WRITE OVER

//MESSAGE SET UP SUCCESS
        printf("message queue 1 made\n");

//CREATE OR OVERWRITE A LOGFILE
        FILE * logfile;
        logfile=fopen(filename,"w");
        fclose(logfile);



//MAKE CHILDREN
        printf("creating children\n");
        int limit=20;
        //INITIALIZE DATA FOR LOOP CONTROL
        int count=0;
        int kval=0;
        int newProcessTimer;
        while(run==0 && i < 100)
        {
            newProcessTimer = rand()%(maxTimeBetweenNewProcs/2)+1;//TIME BETWEEN NEW PROCESSES
            if(count==limit) 	//ONLY ALLOW MAX LIMIT OF PROCESSES AT A TIME
            {
                printf("waiting\n");
                int id = wait(NULL);	//WAIT WHILE AT MAX LIMIT
                for(int j=0; j<20; j++)
                {
                    if(id==pidArray[j]) processOpen[j] = 0;//FREE BIT ARRAY IF WAITED FOR A CHILD
                }
                count--;
            }

            //INCREMENT LOOP CONTROL
            printf("PROCESS COUNT: %d\n", count);

            for(int k = 0; k<20; k++)//LOOP THROUGH PROCESS BLOCK
            {
                //CHECK IF NANOSECONDS CAN BECOME A SECOND
                if(shm_ptr->processBlock[k].timeInSystem.nanoseconds>1000000000)
                {
                    shm_ptr->processBlock[k].timeInSystem.seconds++;
                    shm_ptr->processBlock[k].timeInSystem.nanoseconds-=1000000000;
                }

                fflush(stdout);
                printf("Checking slot %d\n", k);

                if(processOpen[k]==0) 		//CHECK FOR A FREE SLOT
                {
                    printf("Slot %d filled\n", k);
                    processOpen[k]=1;    //FILL THE SLOT
                    shm_ptr->processBlock[k].processID=k;
                    srand(time(NULL)*k);
                    int randprior = rand()%100 + 1; //CREATE A RANDOM NUMBER FOR PRIORITY
                    int queuenum;//FOR LOGFILE
                    if(randprior<(100*realtime))
                    {
                        queuenum=0;
                        shm_ptr->processBlock[k].priority=0;
                        enqueue(&priority0,k);
                        writeLogQueue(logfile, k, 0);               //PUT INTO REALTIME PROCESS QUEUE
                        shm_ptr->processBlock[k].firstrun = 1;
                        printf("%d sent to Priority 0\n",k);
                    }
                    else
                    {
                        queuenum=1;
                        shm_ptr->processBlock[k].priority=1;
                        enqueue(&priority1,k);
                        writeLogQueue(logfile, k, 1);               //PUT INTO QUEUE 1
                        shm_ptr->processBlock[k].firstrun = 1;
                        printf("%d sent to Priority 1\n",k);
                    }

                    srand(time(NULL));
                    int runtime = rand()%(maxTimeBetweenNewProcs) +1; //CREATE A RANDOM RUNTIME
                    shm_ptr->processBlock[k].timeLeft=runtime;
                    shm_ptr->processBlock[k].timeUsedLast=0;
                    shm_ptr->processBlock[k].timeInSystem.seconds=0; //SET AND CLEAR DATA VALUES
                    shm_ptr->processBlock[k].timeInSystem.nanoseconds=0;
                    printf("%d will run for %d\n", k,runtime);
                    count++;
                    i++;
                    kval=k;//FOR USER PROCESS TO KNOW ITS OWN PID
                    writeLogNewProcess(logfile, k, queuenum, shm_ptr->systemTime.seconds, shm_ptr->systemTime.nanoseconds);
                    childpid = fork();		//CREATE CHILD
                    pidArray[k]=childpid;
                    break;

                }

            }

            if(childpid==0) break;
            if(childpid<0)
            {
                perror("childpid");	//HANDLE FORK ERROR
                exit(1);
            }
            int processRan=1;

            //MAIN DRIVER FOR THE OPERATING SYSTEM WHILE LOOP

            while(count<21 && childpid>0 && processRan==1)
            {
                processRan =0;
                if(shm_ptr->systemTime.nanoseconds>1000000000) //SEE IF NANOSECONDS CAN BECOME SECONDS
                {
                    shm_ptr->systemTime.seconds++;
                    shm_ptr->systemTime.nanoseconds-=1000000000;
                }

/////////////////////////////////////////////////////////////////////////////////////////////////////ROUND ROBIN QUEUE 0
                int pran=0;
                int val = dequeue(&priority0);
                while(val>-1)
                {
                    print_list(priority0);
                    pran=1;
                    fflush(stdout);
                    printf("DEQUEUE: %d\n", val);
                    message[val]->timeToRun = 1000000; //one millisecond
                    message[val]->running=1;
                    msgsnd(msqid, message[val], MSG, 0);
                    int dispatch = rand()%10000+100;
                    shm_ptr->systemTime.nanoseconds += dispatch;
                    writeLogRunProcess(logfile, val, 0, shm_ptr->systemTime.seconds, shm_ptr->systemTime.nanoseconds, dispatch);
                    msgrcv(msqid, message[val+20], MSG, val+21,0);
                    if(shm_ptr->processBlock[val].timeLeft < 0 || message[val+20]->running==0)//IF PROCESS RUNS OUT OF TIME OR ENDS ITSELF
                    {
                        shm_ptr->systemTime.nanoseconds += shm_ptr->processBlock[val].timeLeft;
                        shm_ptr->processBlock[val].timeUsedLast = shm_ptr->processBlock[val].timeLeft;
                        shm_ptr->processBlock[val].timeInSystem.nanoseconds+=shm_ptr->processBlock[val].timeLeft;
                        shm_ptr->processBlock[val].timeLeft = 0;
                        newProcessTimer-=shm_ptr->processBlock[val].timeLeft;
                        printf("%d Ran out of time\n", val);
                        message[val]->running=0;
                        if(message[val+20]->running==1)//IF IT RAN OUT OF TIME, SEND THE KILL MESSAGE
                        {
                            msgsnd(msqid, message[val], MSG, 0);
                            msgrcv(msqid, message[val+20], MSG, val+21, 0);
                        }
                        processOpen[val]=0;
                        writeLogEnd(logfile, val, shm_ptr->processBlock[val].timeInSystem.seconds,shm_ptr->processBlock[val].timeInSystem.nanoseconds);
                    }
                    else //ELSE THE PROCESS HAS NOT RAN OUT OF TIME
                    {
                        enqueue(&priority0,val); //PUT BACK INTO THE QUEUE

                        writeLogQueue(logfile, val, 0);
                        newProcessTimer-=message[val+20]->timeToRun;
                        shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeLeft -= message[val+20]->timeToRun;
                        printf("%d has %d time left\n", val, shm_ptr->processBlock[val].timeLeft);
                        shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                    }
                    if(newProcessTimer<0) break; //IF ITS TIME FOR A NEW PROCESS, BREAK OUT QUEUE 0

                    val=dequeue(&priority0);
                }
                processRan=pran;
                if(newProcessTimer<0) break; //BREAK OUT OF DRIVER IF TIME FOR A NEW PROCESS
///////////////////////////////////////////////////////////////////////////////////////////////////// QUEUE 1 START

                val = dequeue(&BlockQueue); //SEE IF SOMETHING IS IN BLOCKED QUEUE
                if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                {//IF SOMETHING IS IN BLOCKED, BUT ITS TIME IS NOT READY YET, PUT IT BACK IN
                    enqueue(&BlockQueue, val);
                    val = dequeue(&priority1);
                }
                else if(val==-1) val = dequeue(&priority1); //IF NOTHING IS IN BLOCKED QUEUE, DEQUEUE FROM QUEUE 1
                else writeLogUnblock(logfile, val);

                while(val>-1)
                {
                    pran=1;
                    fflush(stdout);
                    printf("DEQUEUE: %d\n", val);
                    message[val]->timeToRun = 2000000; //one millisecond RUNTIME EACH ITERATION
                    message[val]->running=1;
                    msgsnd(msqid, message[val], MSG, 0);
                    int dispatch = rand()%10000+100;
                    shm_ptr->systemTime.nanoseconds += dispatch;
                    writeLogRunProcess(logfile, val, 1, shm_ptr->systemTime.seconds, shm_ptr->systemTime.nanoseconds, dispatch);
                    msgrcv(msqid, message[val+20], MSG, val+21,0);
                    if(shm_ptr->processBlock[val].timeLeft< 0 || message[val+20]->running==0)
                    {
                        if(shm_ptr->processBlock[val].timeLeft< 0)
                        {
                            shm_ptr->systemTime.nanoseconds += shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeUsedLast = shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=shm_ptr->processBlock[val].timeLeft;
                        }
                        else //process wants to terminate itself
                        {
                            shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=message[val+20]->timeToRun;
                        }
                        printf("%d Ran out of time\n", val);
                        message[val]->running=0;
                        if(message[val+20]->running==1)
                        {
                            msgsnd(msqid, message[val], MSG, 0);
                            msgrcv(msqid, message[val+20], MSG, val+21, 0);
                        }
                        processOpen[val]=0;
                        writeLogEnd(logfile, val, shm_ptr->processBlock[val].timeInSystem.seconds,shm_ptr->processBlock[val].timeInSystem.nanoseconds);
                    }
                    else //ran normally
                    {
                        if(message[val+20]->running==2) //IT DECIDED TO BLOCK
                        {
                            enqueue(&BlockQueue,val);
                            writeLogQueue(logfile, val, 4);
                        }
                        else //IT RAN NORMALLY
                        {
                            enqueue(&priority2,val);
                            writeLogQueue(logfile, val, 2);
                        }
                        shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeLeft -= message[val+20]->timeToRun;
                        newProcessTimer-=message[val+20]->timeToRun;
                        printf("%d has %d time left\n", val, shm_ptr->processBlock[val].timeLeft);
                        shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                    }
                    if(newProcessTimer<0) break;

                    val = dequeue(&BlockQueue);
                    if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                    {
                        enqueue(&BlockQueue, val);
                        val = dequeue(&priority1);
                    }
                    else if(val==-1) val = dequeue(&priority1);
                    else writeLogUnblock(logfile, val);


                }

                if(newProcessTimer<0) break;
///////////////////////////////////////////////////////////////////////////////////////////////////// QUEUE 2 START

                val = dequeue(&BlockQueue);
                if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                {
                    enqueue(&BlockQueue, val);
                    val = dequeue(&priority2);
                }
                else if(val==-1) val = dequeue(&priority2);
                else writeLogUnblock(logfile, val);

                while(val>-1)
                {
                    pran=1;
                    fflush(stdout);
                    printf("DEQUEUE: %d\n", val);
                    message[val]->timeToRun = 4000000; //one millisecond
                    message[val]->running=1;
                    msgsnd(msqid, message[val], MSG, 0);
                    int dispatch = rand()%10000+100;
                    shm_ptr->systemTime.nanoseconds += dispatch;
                    writeLogRunProcess(logfile, val, 2, shm_ptr->systemTime.seconds, shm_ptr->systemTime.nanoseconds, dispatch);
                    msgrcv(msqid, message[val+20], MSG, val+21,0);
                    if(shm_ptr->processBlock[val].timeLeft < 0 || message[val+20]->running==0)
                    {
                        if(shm_ptr->processBlock[val].timeLeft< 0)
                        {
                            shm_ptr->systemTime.nanoseconds += shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeUsedLast = shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=shm_ptr->processBlock[val].timeLeft;
                        }
                        else
                        {
                            shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=message[val+20]->timeToRun;
                        }

                        printf("%d Ran out of time\n", val);
                        message[val]->running=0;
                        if(message[val+20]->running==1)
                        {
                            msgsnd(msqid, message[val], MSG, 0);
                            msgrcv(msqid, message[val+20], MSG, val+21, 0);
                        }
                        processOpen[val]=0;
                        writeLogEnd(logfile, val, shm_ptr->processBlock[val].timeInSystem.seconds,shm_ptr->processBlock[val].timeInSystem.nanoseconds);
                    }
                    else
                    {
                        if(message[val+20]->running==2)
                        {
                            enqueue(&BlockQueue,val);
                            writeLogQueue(logfile, val, 4);
                        }
                        else
                        {
                            enqueue(&priority3,val);
                            writeLogQueue(logfile, val, 3);
                        }
                        shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeLeft -= message[val+20]->timeToRun;
                        newProcessTimer-=message[val+20]->timeToRun;
                        printf("%d has %d time left\n", val, shm_ptr->processBlock[val].timeLeft);
                        shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                    }
                    if(newProcessTimer<0) break;

                    val = dequeue(&BlockQueue);
                    if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                    {
                        enqueue(&BlockQueue, val);
                        val = dequeue(&priority2);
                    }
                    else if(val==-1) val = dequeue(&priority2);
                    else writeLogUnblock(logfile, val);

                }
                if(newProcessTimer<0) break;
///////////////////////////////////////////////////////////////////////////////////////////////////// QUEUE 3 START

                val = dequeue(&BlockQueue);
                if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                {
                    enqueue(&BlockQueue, val);
                    val = dequeue(&priority3);
                }
                else if(val==-1) val = dequeue(&priority3);
                else writeLogUnblock(logfile, val);

                while(val>-1)
                {
                    pran=1;
                    fflush(stdout);
                    printf("DEQUEUE: %d\n", val);
                    message[val]->timeToRun = 8000000; //one millisecond
                    message[val]->running=1;
                    msgsnd(msqid, message[val], MSG, 0);
                    int dispatch = rand()%10000+100;
                    shm_ptr->systemTime.nanoseconds += dispatch;
                    writeLogRunProcess(logfile, val, 3, shm_ptr->systemTime.seconds, shm_ptr->systemTime.nanoseconds, dispatch);
                    msgrcv(msqid, message[val+20], MSG, val+21,0);
                    if(shm_ptr->processBlock[val].timeLeft< 0 || message[val+20]->running==0)
                    {
                        if(shm_ptr->processBlock[val].timeLeft< 0)
                        {
                            shm_ptr->systemTime.nanoseconds += shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeUsedLast = shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=shm_ptr->processBlock[val].timeLeft;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=shm_ptr->processBlock[val].timeLeft;
                        }
                        else
                        {
                            shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                            shm_ptr->processBlock[val].timeLeft = 0;
                            newProcessTimer-=message[val+20]->timeToRun;
                        }

                        printf("%d Ran out of time\n", val);
                        message[val]->running=0;
                        if(message[val+20]->running==1)
                        {

                            msgsnd(msqid, message[val], MSG, 0);
                            msgrcv(msqid, message[val+20], MSG, val+21, 0);
                        }
                        processOpen[val]=0;
                        writeLogEnd(logfile, val, shm_ptr->processBlock[val].timeInSystem.seconds,shm_ptr->processBlock[val].timeInSystem.nanoseconds);
                    }
                    else
                    {
                        if(message[val+20]->running==2)
                        {
                            enqueue(&BlockQueue, val);
                            writeLogQueue(logfile, val, 4);
                        }
                        else
                        {
                            enqueue(&priority3,val);
                            writeLogQueue(logfile, val, 3);
                        }
                        shm_ptr->systemTime.nanoseconds += message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeLeft -= message[val+20]->timeToRun;
                        newProcessTimer-=message[val+20]->timeToRun;
                        printf("%d has %d time left\n", val, shm_ptr->processBlock[val].timeLeft);
                        shm_ptr->processBlock[val].timeUsedLast = message[val+20]->timeToRun;
                        shm_ptr->processBlock[val].timeInSystem.nanoseconds+=message[val+20]->timeToRun;
                    }
                    if(newProcessTimer<0) break;

                    val = dequeue(&BlockQueue);
                    if(val>-1 && shm_ptr->processBlock[val].UnblockTime>shm_ptr->systemTime.nanoseconds)
                    {
                        enqueue(&BlockQueue, val);
                        val = dequeue(&priority3);
                    }
                    else if(val==-1) val = dequeue(&priority3);
                    else writeLogUnblock(logfile, val);

                }
                processRan=pran;
                if(newProcessTimer<0) break;
                processRan=pran;
            }///////////////////////////////////////////////////////////////////////////////////////////////////// END MAIN DRIVER

            if(childpid==0) break;

            int id = waitpid(-1, NULL, WNOHANG);
            if(id != 0)
            {
                for(int j=0; j<20; j++)
                {                                                           //WAIT FOR ANY CHILDREN
                    if(id==pidArray[j]) processOpen[j] = 0;
                }
                count--;
            }


        }
//SLAVE PROCESS CODE
        if(childpid==0)
        {
            fprintf(stderr, "SLAVE:I:%d PID:%d PPID:%d CID:%d\n",i, getpid(),getppid(),childpid);
            char indchar[10];
            sprintf(indchar, "%d", kval); //TO SEND CHILD ITS PID NUMBER
            char nchar[10];
            int p=100;
            sprintf(nchar, "%d", p);
            char *s[] = { "./user", indchar, (char *)shm_ptr,filename, NULL}; //SET UP EXEC
            printf("EXECUTING SLAVE\n");
            execvp(s[0], s);
        }


		unsigned long int avgwait;
                unsigned long int avgblock;
                unsigned long int avgrun;


//WAIT FOR ANY REMAINING PROCESSES
        while(1)
        {
            childpid=wait(NULL);
            printf("waited for %d, Press Ctrl-C to End\n", childpid);
		for(int l=0; l<100000; l++){
		avgwait+=shm_ptr->waittime[l];
		if(shm_ptr->waittime[l]==0){
		avgwait=(avgwait/l); break;
		}
		}
for(int l=0; l<100000; l++){
                avgblock+=shm_ptr->blocktime[l];
                if(shm_ptr->blocktime[l]==0){
                avgblock=(avgblock/l); break;
                }
                }
for(int l=0; l<100000; l++){
                avgrun+=shm_ptr->runtime[l];
                if(shm_ptr->runtime[l]==0){
                avgrun=(avgrun/l); break;
                }
                }

		printf("AVG RUN:%ld ns\n AVG IDLE:%ld ns\n AVG BLK: %ld ns\n", avgrun, avgwait, avgblock);
		logfile=fopen("AVERAGESTATS.txt","w");
        fprintf(logfile, "AVG RUN:%ld ns\n AVG IDLE:%ld ns\n AVG BLK: %ld ns\n", avgrun, avgwait, avgblock);
        fclose(logfile);  

            if(childpid==-1) break;
        }


	logfile=fopen("AVERAGESTATS.txt","w");
	fprintf(logfile, "AVG RUN:%ld ns\n AVG IDLE:%ld ns\n AVG BLK: %ld ns\n", avgrun, avgwait, avgblock);
	fclose(logfile);

//FREE UP MEMORY
        msgctl(msqid, IPC_RMID, NULL);		//FREE SHARED MESSAGE QUEUE
        shmctl(shm_id, IPC_RMID, NULL);		//FREE SHARED MEMORY DATA

//DONE
        printf("SIMULATION COMPLETE\n");
        return 0;
    }

}

