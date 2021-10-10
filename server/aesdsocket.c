/*
*  aesdsocket.c - source file for implementing assignment 5 part 1 functionality
*  Attributes:
*  https://beej.us/guide/bgnet/html/
*  https://stackoverflow.com/questions/12721246/reading-the-socket-buffer
*  https://stackoverflow.com/questions/8436898/realloc-invalid-next-size-when-reallocating-to-make-space-for-strcat-on-char
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

// macro definition for  buffer size
#define BUFSIZE 600
// macro definiton for port#
#define PORT "9000"
// macro definition for pending connections
#define BACKLOG 1000

/*
*   Structure to hold metadata associated with file
*/
typedef struct{

    pthread_t threaddec;               // thread definition
    int threadIdx;                     // thread ID
    int fd;                            // writing file descriptor
    int sock;                          // client scoket
    
    // Read & Write buffers
    char* read_buf;                    
    char* write_buf;
    sigset_t mask;

    pthread_mutex_t *lock;
    bool completion_status;

}threadParams_t;


// Singly Linked list data strcuture
typedef struct slist_data_s slist_data_t;
struct slist_data_s{

    threadParams_t threadParams;
    SLIST_ENTRY(slist_data_s) entries;
};


slist_data_t *datap = NULL;
SLIST_HEAD(slisthead,slist_data_s) head;

// Variable to start or stop accepting connections
int shutoff=0;

int fd; 

// mutex to protect concurrent file access
pthread_mutex_t file_mutex;

pid_t check;
int sock_t;

timer_t timerid;

int newsock_t;

struct addrinfo host;
struct addrinfo *servinfo;

struct sockaddr_in con_addr;

// Data structure for SIGSEV_THREAD with POSIX timer
typedef struct sigsev_data{

    int fd;            // file descriptor

}sigsev_data;


void close_graceful();

static inline void timespec_add( struct timespec *result,
                        const struct timespec *ts_1, const struct timespec *ts_2)
{
    result->tv_sec = ts_1->tv_sec + ts_2->tv_sec;
    result->tv_nsec = ts_1->tv_nsec + ts_2->tv_nsec;
    if( result->tv_nsec > 1000000000L ) {
        result->tv_nsec -= 1000000000L;
        result->tv_sec ++;
    }
}


/*
* SIGSEV_THREAD to write timestamp to file every 10 seconds
*
*/
static void timer_thread(union sigval sigval){

    struct sigsev_data* td = (struct sigsev_data*) sigval.sival_ptr;

    char buffer[100];
    time_t rtime;
    struct tm *info;
    time(&rtime);
    info = localtime(&rtime);                // Read realtime

    size_t size = strftime(buffer,100,"timestamp:%a, %d %b %Y %T %z\n",info);

    if (pthread_mutex_lock(&file_mutex) != 0){

        perror("Error locking muetx:");
        close_graceful();
        exit(-1);
    }

    // Write to file
    int wbytes = write(td->fd,buffer,size);
    if (wbytes == -1){
        perror("\nERROR write():");
        close_graceful();
        exit(-1);
    }

    if (pthread_mutex_unlock(&file_mutex) != 0){

        perror("Error ulocking muetx:");
        close_graceful();
        exit(-1);
    }

}


void close_graceful(){

    shutoff = 1;

    // close listening server socket fd
    close(sock_t);

    // close file descriptor
    close(fd);

    // Delete file
    if(remove("/var/tmp/aesdsocketdata") != 0){
        syslog(LOG_ERR,"\nERROR in deleting file");
    }
    

    
    // Cancel threads &free pointers
    SLIST_FOREACH(datap,&head,entries){

        if (datap->threadParams.completion_status != true){

            pthread_cancel(datap->threadParams.threaddec);
            free(datap->threadParams.read_buf);
            free(datap->threadParams.write_buf);
            
        }


    }
    
    // free Linked list
    while(!SLIST_EMPTY(&head)){
        datap = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head,entries);
        free(datap);
    }

    pthread_mutex_destroy(&file_mutex);                       // destroy mutex

    timer_delete(timerid);                                    // delete timer

     // close log
    closelog();
    
}


static void sig_handler(int signo){


    if(signo == SIGINT || signo==SIGTERM) {


    shutdown(sock_t,SHUT_RDWR);

    shutoff = 1;

    
    }

}


void Send_Receive(void *threadp){

    char *ch;

    threadParams_t *threadsock = (threadParams_t*)threadp;

    int buff_pos=0;

    ssize_t rbytes;

    int currbuf_size=BUFSIZE;

    // Buffer declarations
    char *temp_buf,*out_buf;

    // allocate read write buffers
    threadsock->read_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    threadsock->write_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    
     
    int num_bytes,wbytes;
    // Read till packet is complete
    while((num_bytes = recv(threadsock->sock,threadsock->read_buf+buff_pos, BUFSIZE, 0))>0){
        
        
        if(num_bytes == -1){
        perror("ERROR recv():");
        close_graceful();
        exit(-1);
        
        }

        buff_pos += num_bytes;
        
        // dynamicall increase buffer size for incoming packets
        if (buff_pos >= currbuf_size){

        currbuf_size+=BUFSIZE;

        temp_buf = realloc(threadsock->read_buf,sizeof(char)*currbuf_size);

        if(temp_buf == NULL){
            syslog(LOG_ERR,"\nErrror in realloc");
            close_graceful();
            exit(-1);
        }

        else threadsock->read_buf = temp_buf;
        
        }
        
        // Search for NULL character
        ch = strchr(threadsock->read_buf,'\n');

        if(ch != NULL) break;
        
    }


    // mutex lock
    if (pthread_mutex_lock( threadsock->lock) != 0){

        perror("Error locking muetx:");
        close_graceful();
        exit(-1);

    }

    // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&(threadsock->mask),NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }

    // Write to file
    wbytes = write(threadsock->fd,threadsock->read_buf,buff_pos);
    if (wbytes == -1){
        perror("\nERROR write():");
        close_graceful();
        exit(-1);
    }

    // Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&(threadsock->mask),NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }


    if(pthread_mutex_unlock(threadsock->lock) != 0){

        perror("Error locking muetx:");
        close_graceful();
        exit(-1);

    }



    lseek(threadsock->fd,0,SEEK_SET);

    int index = 0;
    int drift=0;
    char single_byte;
    int outbuf_size = BUFSIZE;

    // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&(threadsock->mask),NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }

    // mutex lock
    if (pthread_mutex_lock( threadsock->lock) != 0){

        perror("Error locking muetx:");
        close_graceful();
        exit(-1);

    }

    
    // Read one byte at a time from file until new line is found 
    // and send packet by packet with realloc if necessary 
    while((rbytes = read(threadsock->fd,&single_byte,1)) > 0){

        if(rbytes <0 ) {

             perror("\nERROR sigprocmask():");
             close_graceful();
             exit(-1);

        }

        threadsock->write_buf[index] = single_byte;

        if(threadsock->write_buf[index] == '\n'){

            // Send packets
            int packet_size = index - drift + 1;

            if (send(threadsock->sock,threadsock->write_buf+drift,packet_size, 0) == -1){ 
                perror("send");
                close_graceful();
            }

            drift = index + 1;

        }

        index++;

        if(index >= outbuf_size){
            
            outbuf_size += BUFSIZE;
            out_buf=realloc(threadsock->write_buf,sizeof(char)*outbuf_size);
            threadsock->write_buf=out_buf;

        }


    }

    // Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&(threadsock->mask),NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }


    if (pthread_mutex_unlock(threadsock->lock) != 0){

        perror("Error locking muetx:");
        close_graceful();
        exit(-1);
    }


    close(threadsock->sock);

    //status true
    threadsock->completion_status = true;

    free(threadsock->read_buf);
    free(threadsock->write_buf);
}



int main(int argc, char* argv[])
{

    if (pthread_mutex_init(&file_mutex,NULL) != 0){
        perror("Error mutex init():");
        close_graceful();
        exit(-1);
    } 

    SLIST_INIT(&head);

    socklen_t len;

    // To ensure struct is empty
    memset(&host, 0, sizeof(host)); 

    host.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    host.ai_socktype = SOCK_STREAM; // TCP stream sockets
    host.ai_flags = AI_PASSIVE;     // Self IP

    openlog("AESDSOCKET :",LOG_PID,LOG_USER);

    check = getaddrinfo(NULL, PORT, &host, &servinfo);

    if (check != 0){
       perror("\nERROR ADDRINFO():");
        close_graceful();
        exit(-1);
    }
    
    // open socket
    sock_t = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if(sock_t == -1) {
        perror("\nERROR socket():");
        close_graceful();
        exit(-1);
    }

    int reuse_addr = 1;

    if (setsockopt(sock_t, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) == -1) {
        syslog(LOG_ERR, "setsockopt");
        close(sock_t);
        close_graceful();
        exit(-1);
    } 

    // bind onto the socket
    check = bind(sock_t,servinfo->ai_addr, servinfo->ai_addrlen);

    if (check == -1){
        perror("\nERROR bind():");
        freeaddrinfo(servinfo);
        close_graceful();
        exit(-1);
    }

        
    // free address structure
    freeaddrinfo(servinfo);

    // Setup SIGINT signal handler
    if(signal(SIGINT,sig_handler) == SIG_ERR){
        syslog(LOG_ERR,"\nError in setting signals");
        close_graceful();
        exit(-1);
    }
    
    // Setup SIGTERM signal handler
    if(signal(SIGTERM,sig_handler) == SIG_ERR){
        syslog(LOG_ERR,"\nError in setting signals");
        close_graceful();
        exit(-1);
    }

    // Setup signal mask
    sigset_t set;
    sigemptyset(&set);           // empty the set

    // add the signals to the set
    sigaddset(&set,SIGINT);      
    sigaddset(&set,SIGTERM);     

    
    // lisiten for connections
    check = listen(sock_t, BACKLOG);

    if (check == -1){

        perror("\nERROR listen():");
        close_graceful();
        exit(-1);

    }

    fd =  open("/var/tmp/aesdsocketdata",O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
    if(fd<0){
        perror("\nERROR open():");
        close_graceful();
        exit(-1);
    }
    
    // if correct parameter is passed to code start daemon
    if (argc == 2){

        if (!strcmp("-d",argv[1])){

        check = fork();
        if (check == -1){

            perror("\nERROR fork():");
            close_graceful();
            exit(-1);
        }

        if (check != 0){
            exit(0);
        }

        setsid();

        chdir("/");

        open("/dev/null", O_RDWR);
		dup(0);
		dup(0);

    }

    }

    struct sigevent sev;

    sigsev_data td;
    td.fd = fd;

    memset(&sev,0,sizeof(struct sigevent));


    /*
    * Setup a call to timer_thread passing in the td structure as the sigev_value
    * argument
    */
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = &td;
    sev.sigev_notify_function = timer_thread;
    if ( timer_create(CLOCK_MONOTONIC,&sev,&timerid) != 0 ) {
        perror("Error in creating newtimer\n");
    }
    struct timespec start_time;

     if ( clock_gettime(CLOCK_MONOTONIC,&start_time) != 0 ) {
        perror("Error in getting time\n");
    } 

    struct itimerspec itimerspec;
    itimerspec.it_interval.tv_sec = 10;
    itimerspec.it_interval.tv_nsec = 0;

    timespec_add(&itimerspec.it_value,&start_time,&itimerspec.it_interval);
    
    // start timer in child
    if( timer_settime(timerid, TIMER_ABSTIME, &itimerspec, NULL ) != 0 ) {
        perror("Error in setting time\n");
    } 



    while(!shutoff) {


    len = sizeof(con_addr);

   // accept the connection
    newsock_t = accept(sock_t,(struct sockaddr *)&con_addr,&len);

    if(shutoff) break;

    if (newsock_t == -1 ){
        perror("\nERROR accept():");
    }

    char *ip_string = inet_ntoa(con_addr.sin_addr);
    //printf("\nAccepted connection from %s",ip_string);
    syslog(LOG_DEBUG,"\nAccepted connection from %s",ip_string);

    // Assign value to thread meta data structure members
    datap = (slist_data_t*)malloc(sizeof(slist_data_t));
    SLIST_INSERT_HEAD(&head,datap,entries);                                          // add to linked list
    datap->threadParams.sock = newsock_t;
    datap->threadParams.completion_status = false;
    datap->threadParams.fd=fd;
    datap->threadParams.mask = set;
    datap->threadParams.lock = &file_mutex;
    

    // Spawn threads fro each new connection
    if (pthread_create(&(datap->threadParams.threaddec),(void*)0,(void*)&Send_Receive,(void*)&(datap->threadParams)) != 0){

        perror("Error creating thread:");
        close_graceful();
        exit(-1);
    }

    SLIST_FOREACH(datap,&head,entries){
        
        if(datap->threadParams.completion_status == false){

            continue;                           //continue accepting connections

        }

        else if (datap->threadParams.completion_status == true){
            
            // Join completed threads
            pthread_join(datap->threadParams.threaddec,NULL);

        }

    }

}

close_graceful();
return 0;

}