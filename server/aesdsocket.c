#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <fcntl.h>

#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include <syslog.h>

/*Assignment 6 Thread Implementation*/
#include <pthread.h>
#include "queue.h"
#include <time.h>

struct threadSocketSt
{
    /*File Description*/
    int fileDescriptor;
    /*Client Socket*/
    int clientSocket;
    /*Thread Completion Status*/
    bool threadCompletionStatus;
};

struct slist_data_s
{
    // int value;
    pthread_t socketThreadInstance;
    struct threadSocketSt socketParamters;
    SLIST_ENTRY(slist_data_s)
    entries;
};

#define PORT 9000 // the port users will be connecting to

#define BUFFER_STD_SIZE 256

/*File Descriptor for /var/tmp/aesdsocketdata*/
int fd = 0;
/*Server Socket*/
int serverSocket;

/* Signal Handler */
void sig_handler(int signum);

bool deamonFlag = false;

pid_t pid;

bool signalExitFlag = false;

pthread_mutex_t mutexSocket;

/*Socket Processing Function*/
void *socketThreadProcessing(void *ptr);

void timerInit(int *fd, timer_t *timerId);
void handle();

typedef enum
{
    CREATE_SERVER_SOCKET,
    BIND_SOCKET,
    CHECK_DEAMON,
    LISTEN_SOCKET,
    FILE_OPS,
    STATE_ACCEPTING,
    RECV_CLIENT,
    WRITE_DATA_TO_FILE,
    READ_DATA_FROM_FILE,
    SEND_CLIENT,
    THREAD_EXIT,
    CLOSE_CLIENT_SOCKET,
    STATE_SIGNAL_EXIT
} stateVarForSocket;

stateVarForSocket state = CREATE_SERVER_SOCKET;

/*Malloc Counters*/
int mallocCounter1 = 0;
int mallocCounter2 = 0;
int mallocCounter3 = 0;




int main(int argc, char **argv)
{

    openlog("Server3", LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "***************************************** \n");
    printf("Pid of Process in Main = %d************\n", getpid());
    syslog(LOG_INFO, "Pid of Process in Main = %d************\n" , getpid());
    syslog(LOG_INFO, "Server Operations\n");

    syslog(LOG_INFO, "Registering Callbacks for Signal Handlers\n");
    signal(SIGINT, sig_handler);  // Register signal handler
    signal(SIGTERM, sig_handler); // Register signal handler
    // signal(SIGALRM,sig_handler); // Register signal handler

    // alarm(10);
    timer_t timeTest;
    bool timerStart = false;

    /*Deamon Process Check*/
    if (argc > 2)
    {
        perror("Additional Arguments received\n");
        exit(-1);
    }

    if (argc == 2)
    {
        printf("Number of Arguments = %d*******************\n", argc);
        for (int i = 0; i < argc; i++)
        {
            printf("%s\n", argv[i]);
        }
        if (!strcmp(argv[1], "-d"))
        {
            syslog(LOG_INFO, "[+] Deamon Process Ignition\n");
            deamonFlag = true;
        }
    }

    /*Initializing Mutex*/
    int mutexInitialization = pthread_mutex_init(&mutexSocket, NULL);
    if (mutexInitialization < 0)
    {
        perror("[-] Mutex Initialization Failed\n");
        exit(-1);
    }

    int bindSocketRetVal;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    int clientSocket;
    socklen_t addr_size;

    /*State Machine For Socket Communication*/
    while (1)
    {
        switch (state)
        {
        case CREATE_SERVER_SOCKET:
            /*Domain, Type, Protocol*/
            syslog(LOG_INFO, "STATE 1: CREATE_SERVER_SOCKET");
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);

            if (serverSocket < 0)
            {
                perror("[-] Socket Creation Failed\n");
                state = STATE_SIGNAL_EXIT;
            }
            syslog(LOG_INFO, "[+] Server Socket Created\n");

            memset(&serverAddress, '\0', sizeof(struct sockaddr_in));
            memset(&clientAddress, '\0', sizeof(struct sockaddr_in));

            //set socket options
            if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
            {
                perror("error in setsockopt");
                state = STATE_SIGNAL_EXIT;
                break;
            }

            serverAddress.sin_addr.s_addr = INADDR_ANY;
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(PORT);

            state = BIND_SOCKET;
            break;

        case BIND_SOCKET:
        syslog(LOG_INFO,"STATE 2: BIND_SOCKET");
            bindSocketRetVal = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));

            if (bindSocketRetVal < 0)
            {
                perror("[-] Bind Failed\n");
                state = STATE_SIGNAL_EXIT;
            }

            syslog(LOG_INFO, "[+] Bind Socket Completed\n");

            if(deamonFlag == true)
            {
                state = CHECK_DEAMON;
            }
            else
            {
                state = LISTEN_SOCKET;
            }

            
            break;

        case CHECK_DEAMON:
            syslog(LOG_INFO,"STATE 3: CHECK_DEAMON");

                state = LISTEN_SOCKET;

                pid = fork();

                if (pid == -1)
                {
                    perror("Forking Failed\n");
                    return -1;
                }
                else if (pid != 0)
                {
                    printf("Pid of Parent = %d************\n", getpid());
                    syslog(LOG_INFO, "Pid of Parent = %d************\n", getpid());
                    syslog(LOG_INFO, "[+] Parent Process Exiting\n");
                    exit(EXIT_SUCCESS);
                }

                if (setsid() == -1)
                {
                    perror("Set Sid Failed\n");
                    return -1;
                }
                /* set the working directory to the root directory */
                if (chdir("/") == -1)
                {
                    perror("Changing Working Directory Failed\n");
                    return -1;
                }

                close(STDOUT_FILENO);
                close(STDIN_FILENO);
                close(STDERR_FILENO);
                /* close all open files--NR_OPEN is overkill, but works */
                // for (int i = 0; i < NR_OPEN; i++)
                // {
                //     close(i);
                // }
                /* redirect fd's 0,1,2 to /dev/null */
                open("/dev/null", O_RDWR);
                /* stdin */
                dup(0);
                /* stdout */
                dup(0);
                /* stderror */
                /* do its daemon thing... */
                // return 0;

                syslog(LOG_INFO, "[+] Daemon Started\n");
            break;

        case LISTEN_SOCKET:
        syslog(LOG_INFO,"STATE 4: LISTEN_SOCKET");
            /*Server Socket Listening to Connections*/
            int listenRetVal = listen(serverSocket, 5);

            if (listenRetVal < 0)
            {
                perror("[-] Listen Failed\n");
                state = STATE_SIGNAL_EXIT;
            }

            state = FILE_OPS;
            break;

        case FILE_OPS:
        syslog(LOG_INFO,"STATE 5: FILE_OPS");
            fd = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_TRUNC, 0644);
            if (fd == -1)
            {
                perror("File Operation Failed\n");
                state = STATE_SIGNAL_EXIT;
            }


            SLIST_HEAD(slisthead, slist_data_s)
            socketHead;

            SLIST_INIT(&socketHead);

            state = STATE_ACCEPTING;
            break;

        case STATE_ACCEPTING:
            printf("Pid of Process = %d************\n", getpid());
            syslog(LOG_INFO, "Pid of Process = %d************\n", getpid());
            syslog(LOG_INFO, "STATE 6: STATE_ACCEPTING");
            if ((deamonFlag == false) || (pid == 0))
            {
                if (timerStart == false)
                {
                    timerInit(&fd, &timeTest);
                    //timer_init(&fd, &timeTest);
                    timerStart = true;
                }
            }

            addr_size = sizeof(clientAddress);
            clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addr_size);

            if (clientSocket < 0)
            {
                perror("[-] Accept Failed\n");
                state = STATE_SIGNAL_EXIT;
                break;
            }

            //syslog(LOG_INFO, "[+] Client Socket = %d\n", clientSocket);

            struct slist_data_s *socketThreadProcessingStructure = (struct slist_data_s *)malloc(sizeof(struct slist_data_s));
            mallocCounter1++;

            socketThreadProcessingStructure->socketParamters.fileDescriptor = fd;
            socketThreadProcessingStructure->socketParamters.clientSocket = clientSocket;
            socketThreadProcessingStructure->socketParamters.threadCompletionStatus = false;

            pthread_create(&socketThreadProcessingStructure->socketThreadInstance, NULL, socketThreadProcessing,
                           (void *)&socketThreadProcessingStructure->socketParamters);

            SLIST_INSERT_HEAD(&socketHead, socketThreadProcessingStructure, entries);

            pthread_join(socketThreadProcessingStructure->socketThreadInstance, NULL);

            state = CLOSE_CLIENT_SOCKET;
            break;

        case CLOSE_CLIENT_SOCKET:

            syslog(LOG_INFO,"STATE 12: CLOSE_CLIENT_SOCKET");

            SLIST_FOREACH(socketThreadProcessingStructure, &socketHead, entries)
            {
                if (socketThreadProcessingStructure->socketParamters.threadCompletionStatus == true)
                {
                    //syslog(LOG_INFO, "Stage 1\n");

                    close(socketThreadProcessingStructure->socketParamters.clientSocket);
                    SLIST_REMOVE(&socketHead, socketThreadProcessingStructure, slist_data_s, entries);
                    if(socketThreadProcessingStructure != NULL)
                    {
                        free(socketThreadProcessingStructure);
                        //socketThreadProcessingStructure = NULL;
                        
                    }

                    //syslog(LOG_INFO, "Stage 2\n");
                }
                
            }
            mallocCounter1--;
            socketThreadProcessingStructure = NULL;

            state = STATE_ACCEPTING;

            if (signalExitFlag == true)
            {
                //timer_delete(timeTest);
                state = STATE_SIGNAL_EXIT;
                //break;
            }
            break;

        case STATE_SIGNAL_EXIT:

            syslog(LOG_INFO, "STATE 13: CLOSE_CLIENT_SOCKET");
            /*Do all the exit cleanup over here*/
            /*File Descriptor for the /var/tmp/aesdsocketdata*/
            if (close(fd) != 0)
            {
                printf("File Closing Failed************\n");
            }


            if (pthread_mutex_destroy(&mutexSocket) != 0)
            {
                perror("[-] Mutex Destroy Failed\n");
            }

            if (remove("/var/tmp/aesdsocketdata") == -1)
            {
                perror("[-] File Delete Failed\n");
            }

            /*Server Socket*/
            if (close(serverSocket) != 0)
            {
                printf("Socket Closing Failed************\n");
            }

            if(mallocCounter1 > 0)
            {
                SLIST_FOREACH(socketThreadProcessingStructure, &socketHead, entries)
                {
                    close(socketThreadProcessingStructure->socketParamters.clientSocket);
                    SLIST_REMOVE(&socketHead, socketThreadProcessingStructure, slist_data_s, entries);
                    // if(socketThreadProcessingStructure != NULL)
                    // {
                        free(socketThreadProcessingStructure);
                        mallocCounter1--;
                    //}
                    //socketThreadProcessingStructure = NULL;
                    
                }
            }
                socketThreadProcessingStructure = NULL;

        

            //printf("Exiting Program....\n");

            syslog(LOG_INFO, "Exiting Program\n");
            syslog(LOG_INFO, "**************************************** \n");

            /*Closing Syslog File*/
            

            syslog(LOG_INFO, "Malloc Counter 1 = %d\n", mallocCounter1);
            syslog(LOG_INFO, "Malloc Counter 2 = %d\n", mallocCounter2);
            syslog(LOG_INFO, "Malloc Counter 3 = %d\n", mallocCounter3);

            closelog();

            /*Hard Exit with -1 as return value*/
            return -1;

            break;

        default:
            break;
        }
    }

    return 0;
}

void sig_handler(int signum)
{
    printf("Signal Handler Start\n");
    syslog(LOG_INFO, "Signal Hanlder Start\n");

    if (signum == SIGINT)
    {
        syslog(LOG_INFO, "Caught signal = %d, exiting\n", SIGINT);
    }
    else if (signum == SIGTERM)
    {
        printf("Caught signal = %d, exiting\n", SIGTERM);
    }

    /*TODO: Include a flag and handle the flag exit conditions in main*/
    /*Suggestion by Dan Walkes in Lectures*/
    if (shutdown(serverSocket, SHUT_RDWR))
    {
        perror("[-] Failed on Shutdown\n");
        syslog(LOG_ERR, "Could not close socket file descriptor in signal handler : %s", strerror(errno));
    }

    signalExitFlag = true;

    printf("Signal Handler End\n");
    syslog(LOG_INFO, "Signal Hanlder End\n");

}

/*Thread Function for Sockets*/

void *socketThreadProcessing(void *ptr)
{

    int totalBuffer = BUFFER_STD_SIZE;
    int currentSize  = 0;
    char *writebuffer = NULL;
    char *readFromFile = NULL;
    int bytesToBeWritten = 0;
    int lseekValue = 0;
    int bytesToBeRead = 0;
    int recvClient = 0;
    char bufferClientReceive[BUFFER_STD_SIZE];

    /*Thread Parameters*/
    struct threadSocketSt *threadParam = (struct threadSocketSt *)ptr;

    writebuffer = (char*)malloc(sizeof(char) * BUFFER_STD_SIZE);
    if (writebuffer == NULL)
    {
        perror("[-] Malloc for Write Buffer Failed\n");
        state = THREAD_EXIT;
    }
    mallocCounter2++;

    state = RECV_CLIENT;

    /*Implement state Machine*/
    while (1)
    {
        switch (state)
        {
        case RECV_CLIENT:
        syslog(LOG_INFO , "STATE 7: RECV_CLIENT\n");
            bool exitLoop = true;

            while (exitLoop)
            {
                recvClient = recv(threadParam->clientSocket, bufferClientReceive, BUFFER_STD_SIZE, 0);

                if (recvClient == 0 || (strchr(bufferClientReceive, '\n') != NULL))
                {
                    syslog(LOG_INFO, "Packet Completed\n");
                    exitLoop = false;
                }

                if ((totalBuffer - currentSize) < recvClient)
                {
                    totalBuffer += recvClient;
                    writebuffer = (char *)realloc(writebuffer, sizeof(char) * totalBuffer);
                }

                memcpy(writebuffer + currentSize, bufferClientReceive, recvClient);
                currentSize += recvClient;
            }

            state = WRITE_DATA_TO_FILE;
            break;

        case WRITE_DATA_TO_FILE:
        syslog(LOG_INFO , "STATE 8: WRITE_DATA_TO_FILE\n");

            pthread_mutex_lock(&mutexSocket);
            bytesToBeWritten = write(threadParam->fileDescriptor, writebuffer, currentSize);
            lseekValue = lseek(threadParam->fileDescriptor, 0, SEEK_SET);
            pthread_mutex_unlock(&mutexSocket);

            state = READ_DATA_FROM_FILE;
            break;

        case READ_DATA_FROM_FILE:
        syslog(LOG_INFO , "STATE 9: READ_DATA_FROM_FILE\n");
            pthread_mutex_lock(&mutexSocket);
            int lastCharOfFile = lseek(threadParam->fileDescriptor, 0, SEEK_END);
            pthread_mutex_unlock(&mutexSocket);

            printf("Last Char of the File = %d*****************\n", lastCharOfFile);

            readFromFile = (char *)malloc(sizeof(char) * lastCharOfFile);

            if (readFromFile == NULL)
            {
                perror("[-] Malloc Failed\n");
                state = THREAD_EXIT;
                break;

            }

            mallocCounter3++;

            pthread_mutex_lock(&mutexSocket);
            lseek(threadParam->fileDescriptor, 0, SEEK_SET);
            bytesToBeRead = read(threadParam->fileDescriptor, readFromFile, lastCharOfFile);
            pthread_mutex_unlock(&mutexSocket);

            printf("Bytes Reading Using Seek End = %d\n", bytesToBeRead);

            state = SEND_CLIENT;
            break;

        case SEND_CLIENT:
        syslog(LOG_INFO , "STATE 10: SEND_CLIENT\n");
            int clientSendRetVal = send(threadParam->clientSocket, readFromFile, bytesToBeRead, 0);

            // syslog(LOG_INFO, "Client Send Return Value = %d\n", clientSendRetVal);

            if (clientSendRetVal < 0)
            {
                perror("[-] Client Sending Failed\n");
                state = THREAD_EXIT;
                //break;
            }

            free(readFromFile);
            mallocCounter2--;
            free(writebuffer);
            mallocCounter3--;

            if (close(threadParam->clientSocket) != 0)
            {
                printf("Client Socket Closing Failed\n");
            }

            state = THREAD_EXIT; 

            if (signalExitFlag == true)
            {
                state = THREAD_EXIT;
            }
            break;

        case THREAD_EXIT:
        syslog(LOG_INFO , "STATE 11: THREAD_EXIT\n");
            threadParam->threadCompletionStatus = true;
            return NULL;
            break;

        default:
            break;
        }
    }


}

/**********************************/
/*     Timer Functionality        */
/**********************************/
/*Source : Linux System Programming*/
void handle(union sigval sigval)
{
    int *fd = (int *)sigval.sival_ptr;
    struct tm *time_info;
    char time_format[100];
    time_t time_stamp;
    int nwrite;
    size_t time_size;

    time(&time_stamp);
    time_info = localtime(&time_stamp);
    memcpy(time_format, "", 100);
    time_size = strftime(time_format, 100, "timestamp:%a, %d %b %Y %T %z\n", time_info);

    pthread_mutex_lock(&mutexSocket);
    nwrite = write(*fd, time_format, time_size);
    if (nwrite < 0)
    {
        perror("[-] Writing To File Failed");
    }
    pthread_mutex_unlock(&mutexSocket);
}

void timerInit(int *fd, timer_t *timerId)
{
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int ret;
    memset(&evp, 0, sizeof(evp));
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = handle;
    // evp.sigev_value.sival_int = 3; // As an argument to handle()
    evp.sigev_value.sival_ptr = fd;
    ret = timer_create(CLOCK_MONOTONIC, &evp, &timer);
    if (ret)
    {
        perror("timer_create");
    }

    ts.it_interval.tv_sec = 10;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 10;
    ts.it_value.tv_nsec = 0;
    ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL);
    if (ret)
    {
        perror("timer_settime");
    }
}

