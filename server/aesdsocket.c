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

// syslog(LOG_ERR , "Insufficient Arguments");

#define PORT 9000 // the port users will be connecting to

#define BUFFER_STD_SIZE 256

char *localhost = "127.0.0.1";

int fd = 0;
/*Server Socket*/
int serverSocket;

/* Signal Handler */
void sig_handler(int signum);

bool deamonFlag = false;

pid_t pid;

// syslog

int main(int argc, char **argv)
{

    openlog("ServersOpsDB", LOG_CONS, LOG_USER);

    syslog(LOG_INFO, "Server Operations\n");

    syslog(LOG_INFO, "Registering Callbacks for Signal Handlers\n");
    // printf("Registering Callbacks for Signal Handlers\n");
    signal(SIGINT, sig_handler);  // Register signal handler
    signal(SIGTERM, sig_handler); // Register signal handler

    /*File Operations*/
    FILE *filePointer;

    int bindSocket;
    int recvClient;
    int sendClient;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    int clientSocket;
    socklen_t addr_size;

    /*Domain, Type, Protocol*/
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        perror("[-] Socket Creation Failed\n");
        exit(-1);
    }
    syslog(LOG_INFO, "[+] Server Socket Created\n");
    // printf("[+] Server Socket Created\n");

    memset(&serverAddress, '\0', sizeof(struct sockaddr_in));
    memset(&clientAddress, '\0', sizeof(struct sockaddr_in));

    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    bindSocket = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));

    if (bindSocket < 0)
    {
        perror("[-] Bind Failed\n");
        exit(-1);
    }

    syslog(LOG_INFO, "[+] Bind Socket Completed\n");
    // printf("[+] Bind Socket Completed\n");

    if (argc > 2)
    {
        perror("Additional Arguments received\n");
        exit(-1);
    }

    if (argc == 2)
    {
        // printf("Number of Arguments = %d\n", argc);
        // for(int i = 0; i < argc; i++)
        // {
        //     printf("%s\n", argv[i]);

        // }
        if (!strcmp(argv[1], "-d"))
        {
            syslog(LOG_INFO, "[+] Deamon Process Ignition\n");
            // printf("Deamon Process Ignition\n");
            deamonFlag = true;
        }
    }

    if (deamonFlag)
    {
        pid = fork();

        if (pid == -1)
        {
            perror("Forking Failed\n");
            return -1;
        }
        else if (pid != 0)
        {
            syslog(LOG_INFO, "[+] Parent Process Exiting\n");
            // printf("Parent Process Exiting\n");
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
        // perror("Daemon Started\n");
    }

    /*Server Socket Listening to Connections*/
    int listenRetVal = listen(serverSocket, 5);

    if (listenRetVal < 0)
    {
        perror("[-] Listen Failed\n");
    }

    // /*Debug Steps*/
    // printf("Server Address : %d\n", serverAddress.sin_addr.s_addr);
    // printf("Server Family : %d\n", serverAddress.sin_family);
    // printf("Server Port : %d\n", htons(serverAddress.sin_port));

    // char *receiveFromClient;
    char *writebuffer;

    int totalBuffer = BUFFER_STD_SIZE;
    int currentSize = 0;

    char receiveFromClient[BUFFER_STD_SIZE];

    int bytesToBeWritten = 0;
    int readBufferSize = 0;

    char *readFromFile;

    int bytesToBeRead = 0;

    int lseekValue = 0;

    fd = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_TRUNC, 0644);

    while (1)
    {
        addr_size = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addr_size);

        // printf("Client Socket = %d\n", clientSocket);
        syslog(LOG_INFO, "[+] Client Socket = %d\n", clientSocket);

        totalBuffer = BUFFER_STD_SIZE;
        currentSize = 0;

        writebuffer = malloc(sizeof(char) * BUFFER_STD_SIZE);

        // filePointer = fopen("dhiraj.txt", "w");

        if (clientSocket < 0)
        {
            perror("[-] Accept Failed\n");
            exit(-1);
        }

        // printf("Client Connected\n");
        syslog(LOG_INFO, "Accepted connection from : %s\n", inet_ntoa(clientAddress.sin_addr));
        // printf("Client Family : %d\n", clientAddress.sin_family);
        // printf("Client Port : %d\n", clientAddress.sin_port);

        bool exitLoop = true;

        while (exitLoop)
        {
            recvClient = recv(clientSocket, receiveFromClient, BUFFER_STD_SIZE, 0);

            if (recvClient == 0 || (strchr(receiveFromClient, '\n') != NULL))
            {
                syslog(LOG_INFO, "Packet Completed\n");
                exitLoop = false;
            }

            if ((totalBuffer - currentSize) < recvClient)
            {
                totalBuffer += recvClient;
                writebuffer = (char *)realloc(writebuffer, sizeof(char) * totalBuffer);
            }

            memcpy(writebuffer + currentSize, receiveFromClient, recvClient);
            currentSize += recvClient;
        }

        bytesToBeWritten = write(fd, writebuffer, currentSize);
        lseekValue = lseek(fd, 0, SEEK_SET);

        // int fseekValue = fseek(filePointer, 0, SEEK_END);

        // printf("lseekValue  = %d\n", lseekValue);

        // printf("Bytes To be Written = %d\n", bytesToBeWritten);

        readBufferSize += currentSize;

        syslog(LOG_INFO, "Current Size = %d\n", currentSize);
        syslog(LOG_INFO, "Read Bytes = %d\n", readBufferSize);

        readFromFile = (char *)malloc(sizeof(char) * readBufferSize);

        if (readFromFile == NULL)
        {
            perror("[-] Malloc Failed\n");
        }

        // bytesToBeRead = fread(readFromFile , sizeof(char) , readBufferSize, filePointer);

        bytesToBeRead = read(fd, readFromFile, readBufferSize);

        syslog(LOG_INFO, "Bytes To be Read = %d\n", bytesToBeRead);

        int clientSendRetVal = send(clientSocket, readFromFile, bytesToBeRead, 0);

        syslog(LOG_INFO, "Client Send Return Value = %d\n", clientSendRetVal);

        if (clientSendRetVal < 0)
        {
            perror("[-] Client Sending Failed\n");
        }

        free(readFromFile);
        free(writebuffer);

        /*Closing Socket*/
        // int closeSocketRetVal = close(serverSocket);

        // if (closeSocketRetVal < 0)
        // {
        //     perror("[-] Socket Closing Failed\n");
        //     exit(-1);
        // }

        syslog(LOG_INFO, "Closed connection from : %s\n", inet_ntoa(clientAddress.sin_addr));
    }

    return 0;
}

void sig_handler(int signum)
{

    if (signum == SIGINT)
    {
        syslog(LOG_INFO, "Caught signal = %d, exiting\n", SIGINT);
    }
    else if (signum == SIGTERM)
    {
        printf("Caught signal = %d, exiting\n", SIGTERM);
    }

    close(fd);
    close(serverSocket);
    remove("/var/tmp/aesdsocketdata");

    close(STDOUT_FILENO);
    close(STDIN_FILENO);
    close(STDERR_FILENO);

    syslog(LOG_INFO, "Signal Handler Completed\n");

    /*Closing Syslog File*/
    closelog();

    exit(0);
}