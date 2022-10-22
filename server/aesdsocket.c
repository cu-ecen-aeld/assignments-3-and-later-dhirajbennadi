

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/queue.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>


#define USE_AESD_CHAR_DEVICE 1

#ifdef USE_AESD_CHAR_DEVICE
#define WRITE_FILE_PATH "/dev/aesdchar"
#else
#define WRITE_FILE_PATH "/var/tmp/aesdsocketdata"
#endif

#define PORT "9000"
//#define WRITE_FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER 2048
#define BACKLOG 10


static int socket_fd = -1;
struct addrinfo hints, *res;
static pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
bool signal_recv = false;
//char buf[BUFFER];

typedef struct 
{
    int client_fd;
    pthread_t thread;
    pthread_mutex_t* mutex;
    bool thread_complete_status;
} thread_data; 

struct slist_data_s
{
    thread_data   params;
    SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;



/******************************************************************************************/

void clean_all()
{
    if(socket_fd > -1)
    {
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }
    
    pthread_mutex_destroy(&mutex_buffer);
     
    //close the logging	
    closelog();
}

/******************************************************************************************/

static void signal_handler(int sig_num)
{
    syslog(LOG_INFO, "Signal Caught %d\n\r", sig_num);
    signal_recv = true;
    
    if((sig_num == SIGINT) || (sig_num == SIGTERM))
    {
        clean_all();
    }
    exit(0);
}

/******************************************************************************************/

#ifndef USE_AESD_CHAR_DEVICE
void *timer_func(void *args)
{
    size_t buf_len;
    time_t rawtime;
    struct tm *time_local;
    struct timespec request = {0, 0};
    int time_interval = 10; //Timer Interval

    while (!signal_recv)
    {
    
        if (clock_gettime(CLOCK_MONOTONIC, &request))
        {
            syslog(LOG_ERR, "Error: failed to get monotonic time, [%s]\n", strerror(errno));
            continue;
        }
    
        request.tv_sec += 1; 
        request.tv_nsec += 1000000;
    
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &request, NULL) != 0)
        {
            if (errno == EINTR)
            {
                break; 
            }
        }	
    
        if ((--time_interval) <= 0)
      	{
	    char buffer[100] = {0};
    	    time(&rawtime);        
    	    time_local = localtime(&rawtime); 
    	    buf_len = strftime(buffer, 100, "timestamp:%a, %d %b %Y %T %z\n", time_local);
	    int fd = open(WRITE_FILE_PATH, O_RDWR | O_APPEND, 0644);
    
    	    if (fd < 0)
    	    {
    	    	syslog(LOG_ERR, "failed to open a file:%d\n!!!", errno);
    	    }
    
            int rv = pthread_mutex_lock(&mutex_buffer);
    
            if(rv)
            {
                syslog(LOG_ERR, "Error in locking the mutex");
                close(fd);
            }
    
            lseek(fd, 0, SEEK_END); 
    
            int write_bytes = write(fd, buffer, buf_len);
    
            syslog(LOG_INFO, "Timestamp %s written to file\n", buffer);
    	     
            if (write_bytes < 0)
            {
                syslog(LOG_ERR, "Write of timestamp failed errno %d",errno);
            }
    
            rv = pthread_mutex_unlock(&mutex_buffer);
            if(rv)
            {
                syslog(LOG_ERR, "Error in unlocking the mutex\n\r");
                close(fd);
            }
            close(fd);
            time_interval = 10;
        } 
    }
    
    pthread_exit(NULL);

}
#endif
/******************************************************************************************/

void* thread_func(void* thread_params)
{  

    thread_data* params = (thread_data*)thread_params;
    
    char* client_read_buf = (char*)malloc(sizeof(char) * BUFFER);
    
    if(client_read_buf == NULL)
    {
       syslog(LOG_ERR,"malloc failed %d\n\r", (int)(params->thread));
       params->thread_complete_status = true;
    }
    else
    {
       memset(client_read_buf, 0, BUFFER);
    }
    
    uint32_t counter = 1; 
    int curr_pos = 0;


    while(!(params->thread_complete_status))
    {
	int read_bytes = read(params->client_fd, client_read_buf + curr_pos , (BUFFER));
	if (read_bytes < 0) 
	{
	    syslog(LOG_ERR, "reading from socket errno=%d\n", errno);
	    free(client_read_buf);
            params->thread_complete_status = true;
            pthread_exit(NULL);
	}

	if (read_bytes == 0)
	{
	    continue;
	}
	
	curr_pos += read_bytes;

	if (strchr(client_read_buf, '\n')) 
	{  
	    break; 
	} 
	
        counter++;
        client_read_buf = (char*)realloc(client_read_buf, (counter * BUFFER));
       
        if(client_read_buf == NULL)
        {
          syslog(LOG_ERR,"realloc error %d\n\r", (int)params->thread);
          free(client_read_buf);
          params->thread_complete_status = true;
          pthread_exit(NULL);
        }
    }
    
    

    int fd = open(WRITE_FILE_PATH, O_RDWR | O_APPEND, 0644);
    if (fd < 0)
    {
	syslog(LOG_ERR, "failed to open a file:%d\n", errno);
    }

    lseek(fd, 0, SEEK_END);
	
    int rv1 = pthread_mutex_lock(params->mutex);
    if(rv1)
    {
        syslog(LOG_ERR, "Error in locking the mutex\n\r");
        free(client_read_buf);
        params->thread_complete_status = true;
        pthread_exit(NULL);
    }

    int file_write = write(fd, client_read_buf, curr_pos);
    if(file_write < 0)
    {
        syslog(LOG_ERR, "Writing to file error no: %d\n\r", errno);
        free(client_read_buf);
        params->thread_complete_status = true;
        close(fd);
        pthread_exit(NULL);
    }

    lseek(fd, 0, SEEK_SET); 

    rv1 = pthread_mutex_unlock(params->mutex);
    if(rv1)
    {
        syslog(LOG_ERR, "Error in unlocking the mutex\n\r");
        free(client_read_buf);
        params->thread_complete_status = true;
        pthread_exit(NULL);
    }

    close(fd);

    int read_offset = 0;
    
    int fd_dev = open(WRITE_FILE_PATH, O_RDWR | O_APPEND, 0644);
    if(fd_dev < 0)
    {
        free(client_read_buf);
        params->thread_complete_status = true;
        pthread_exit(NULL);    
    }


    lseek(fd_dev, read_offset, SEEK_SET);

    char* client_write_buf = (char*)malloc(sizeof(char) * BUFFER);

    curr_pos = 0;
    
    memset(client_write_buf,0, BUFFER);
    
    counter = 1;
    

    while(1) 
    {
    
        rv1 = pthread_mutex_lock(params->mutex);
        
        if(rv1)
    	{
            syslog(LOG_ERR, "Error in locking the mutex\n\r");
            free(client_read_buf);
            free(client_write_buf);
            params->thread_complete_status = true;
            pthread_exit(NULL);
    	}
    	
    	int read_bytes = read(fd_dev, &client_write_buf[curr_pos], 1);
    	
        rv1 = pthread_mutex_unlock(params->mutex);   
       
	if(rv1)
	{
            free(client_read_buf);
            free(client_write_buf);
            params->thread_complete_status = true;
            pthread_exit(NULL);
	}
	
	if(read_bytes < 0)
	{
           break;
	}

	if(read_bytes == 0)
	{
           break;
	}
	
        if(client_write_buf[curr_pos] == '\n')
        {
           int write_bytes = write(params->client_fd, client_write_buf, curr_pos + 1 );

           if(write_bytes < 0)
           {
              syslog(LOG_ERR, "Error writing to client fd %d\n", errno);
              break;
           }
           memset(client_write_buf, 0, (curr_pos + 1));

           curr_pos = 0;
        } 
        else 
        {
           curr_pos++;
           
           if(curr_pos > sizeof(client_write_buf))
           {
              counter++;
              
              client_write_buf = realloc(client_write_buf, counter * BUFFER);
              
              if(client_write_buf == NULL)
              {
                 free(client_write_buf);
                 free(client_read_buf);
                 params->thread_complete_status = true;
                 pthread_exit(NULL);
              }
           }   
        }
    }    	
        
    close(fd_dev);   
    
    free(client_write_buf);
    free(client_read_buf);     

    params->thread_complete_status = true;
    pthread_exit(NULL);
}

/******************************************************************************************/


//entry point
int main(int argc, char **argv) 
{

    openlog("aesdsocket", 0, LOG_USER);
       
    // register signals 
    sig_t ret_val = signal(SIGINT, signal_handler);   
    if (ret_val == SIG_ERR) 
    {
        syslog(LOG_ERR, "Error while registering SIGINT\n\r");
	clean_all();
    }

    ret_val = signal(SIGTERM, signal_handler);
    if (ret_val == SIG_ERR) 
    {
	syslog(LOG_ERR, "Error while registering SIGTERM\n\r");
	clean_all();
    }

    bool daemon_flag = false;
    
    //check for daemon
    if (argc == 2) 
    {
	if (!strcmp(argv[1], "-d")) 
	{
	    daemon_flag = true;
	} 
	else 
	{
	    printf("wrong arg: %s\nUse -d option for running as daemon", argv[1]);
	    syslog(LOG_ERR, "wrong arg: %s\nUse -d option for running as daemon", argv[1]);
	    exit(EXIT_FAILURE);
	}
    }	
   
    int write_fd = creat(WRITE_FILE_PATH, 0766);
    if(write_fd < 0)
    {
	syslog(LOG_ERR, "aesdsocketdata file could not be created, error number %d", errno);
	clean_all();
	exit(1);
    }
    close(write_fd);

    //Initialize the linked list
    slist_data_t *listPtr = NULL;

    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;	
	
    int result = getaddrinfo(NULL, (PORT), &hints, &res);
    if (result != 0) 
    {
	syslog(LOG_ERR, "getaddrinfo() error %s\n", gai_strerror(result));
	clean_all();
	exit(EXIT_FAILURE);
    }

    //create socket connection
    socket_fd = socket(res->ai_family, SOCK_STREAM, 0);
    if (socket_fd < 0) 
    {
	syslog(LOG_ERR, "socket creation failed, error number %d\n", errno);
	clean_all();
	exit(EXIT_FAILURE);
    }

    // Set sockopts for reuse of server socket
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) 
    {
	syslog(LOG_ERR, "set socket options failed with error number%d\n", errno);
	clean_all();
	exit(EXIT_FAILURE);
    }

    // Bind device address to socket
    if (bind(socket_fd, res->ai_addr, res->ai_addrlen) < 0) 
    {
	syslog(LOG_ERR, "binding socket error num %d\n", errno);
	clean_all();
	exit(EXIT_FAILURE);
    }
	
    freeaddrinfo(res);

    // Listen for connection
    if (listen(socket_fd, BACKLOG)) 
    {
        syslog(LOG_ERR, "listening for connection error num %d\n", errno);
	clean_all();
	exit(EXIT_FAILURE);
    }

    printf("Listening for connections\n\r");

    if (daemon_flag == true) 
    {
	int ret_val = daemon(0,0);
       
	if(ret_val == -1)
	{
	    syslog(LOG_ERR, "failed to create daemon\n");
	    clean_all();
	    exit(EXIT_FAILURE);
	}
    }

#ifndef USE_AESD_CHAR_DEVICE	
    pthread_t timer_thread_id; 
    pthread_create(&timer_thread_id, NULL, timer_func, NULL);
#endif    

    while(!(signal_recv))
    {
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof(client_addr);

	int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        
        if(client_fd < 0)
	{
	    syslog(LOG_ERR, "accepting new connection error is %s", strerror(errno));
	    clean_all();
	    exit(EXIT_FAILURE);
	} 
	   
	if(signal_recv)
	{
            break;
	}
       
	   
        char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);    
	syslog(LOG_INFO, "Accepted connection from %s \n\r",client_ip);
	printf("Accepted connection from %s\n\r", client_ip);
	   
	listPtr = (slist_data_t *) malloc(sizeof(slist_data_t));

        SLIST_INSERT_HEAD(&head, listPtr, entries);

        listPtr->params.client_fd              = client_fd;
        listPtr->params.mutex                  = &mutex_buffer;
        listPtr->params.thread_complete_status = false;

        pthread_create(&(listPtr->params.thread), NULL, thread_func, (void*)&listPtr->params);

        SLIST_FOREACH(listPtr,&head,entries)
        {     
            if(listPtr->params.thread_complete_status == true)
            {
                pthread_join(listPtr->params.thread,NULL);

                shutdown(listPtr->params.client_fd, SHUT_RDWR);

                close(listPtr->params.client_fd);

                syslog(LOG_INFO, "Join spawned thread:%d\n\r",(int)listPtr->params.thread); 
            }
        }
    }

#ifndef USE_AESD_CHAR_DEVICE	
    pthread_join(timer_thread_id, NULL);
#endif    
	
    while (!SLIST_EMPTY(&head))
    {
    listPtr = SLIST_FIRST(&head);
    pthread_cancel(listPtr->params.thread);
    syslog(LOG_INFO, "Thread is killed:%d\n\r", (int) listPtr->params.thread);
    SLIST_REMOVE_HEAD(&head, entries);
    free(listPtr); 
    listPtr = NULL;
    }
	
    if (access(WRITE_FILE_PATH, F_OK) == 0) 
    {
	remove(WRITE_FILE_PATH);
    }
	
    clean_all();
	
    exit(0);
	
}

/******************************************************************************************/
