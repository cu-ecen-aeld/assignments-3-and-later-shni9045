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
#include "queue.h"

// macro definition for  buffer size
#define BUFSIZE 500
// macro definiton for port#
#define PORT "9000"
// macro definition for pending connections
#define BACKLOG 1000

typedef struct{

    pthread_t threaddec;
    int threadIdx;
    int sock;
    char* read_buf;
    char* write_buf;
    bool completion_status;

}threadParams_t;



typedef struct slist_data_s slist_data_t;
struct slist_data_s{

    threadParams_t threadParams;
    SLIST_ENTRY(slist_data_s) entries;
};


slist_data_t *datap = NULL;
SLIST_HEAD(slisthead,slist_data_s) head;



pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

pid_t check;
int sock_t;

int newsock_t;

struct addrinfo host;
struct addrinfo *servinfo;

struct sockaddr_in con_addr;


void close_graceful(){

      // close listening server socket fd
    close(sock_t);

    // close file descriptor
    //close(fd);

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

     // close log
    closelog();
    


}



void Send_Receive(void *threadp){

    int fd;

    char *ch;

    threadParams_t *threadsock = (threadParams_t*)threadp;

    // structure to store file metadata
    struct stat  stat_data;
    int status;


    int num_bytes,wbytes;
    int buff_pos=0;


    ssize_t read_bytes;

    int currbuf_size=BUFSIZE;

    // Buffer declarations
    char *temp_buf,*out_buf;

    // allocate read write buffers
    threadsock->read_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    threadsock->write_buf = (char*)malloc(sizeof(char)*BUFSIZE);


    // Read till packet is complete
    while((num_bytes = recv(newsock_t,threadsock->read_buf+buff_pos, BUFSIZE, 0))>0){

    if(num_bytes == -1){
        perror("ERROR recv():");
        close_graceful();
        exit(-1);    

    }

    buff_pos += num_bytes;
    
    // dynamicall increase buffer size for incoming packets
    //if (buff_pos >= currbuf_size){

    currbuf_size+=BUFSIZE;

    temp_buf = realloc(threadsock->read_buf,sizeof(char)*currbuf_size);

    if(temp_buf == NULL){
        syslog(LOG_ERR,"\nErrror in realloc");
        close_graceful();
        exit(-1);
    }

    else threadsock->read_buf = temp_buf;
    
    //}
    
    // Search for NULL character
    ch = strchr(threadsock->read_buf,'\n');

    if(ch != NULL) break;
    
    
    }

    /*
    // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&set,NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }*/

    // mutex lock
    pthread_mutex_lock(&file_mutex);

    // Open file for writing
    fd = open("/var/tmp/aesdsocketdata",O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
    if(fd<0){
        perror("\nERROR open():");
        close_graceful();
        exit(-1);
    }

    // Write to file
    wbytes = write(fd,threadsock->read_buf,buff_pos);
    if (wbytes == -1){
        perror("\nERROR write():");
        close_graceful();
        exit(-1);
    }
    
    // Extract file size
    status = stat("/var/tmp/aesdsocketdata",&stat_data);
    int si;
    if (status == 0){
    si=stat_data.st_size;

    }

    out_buf=realloc(threadsock->write_buf,sizeof(char)*si);
    threadsock->write_buf=out_buf;


    lseek(fd,0,SEEK_SET);

    read_bytes = read(fd,threadsock->write_buf,si);

    pthread_mutex_unlock(&file_mutex);
    
    // Send packets
    if (send(newsock_t,threadsock->write_buf,read_bytes, 0) == -1){
    perror("send");
    close_graceful();
    }

    /*// Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&set,NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }*/

    close(threadsock->sock);

    //status true
    threadsock->completion_status = true;

    free(threadsock->read_buf);
    free(threadsock->write_buf);
}

static void sig_handler(int signo){


    // close listening server socket fd
    close(sock_t);

    // close file descriptor
    //close(fd);

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

     // close log
    closelog();
    
    exit(0);


}

int main(int argc, char* argv[])
{

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

    //int num_threads = 0;

    while(1) {


    len = sizeof(con_addr);

   // accept the connection
    newsock_t = accept(sock_t,(struct sockaddr *)&con_addr,&len);

    if (newsock_t == -1 ){
        perror("\nERROR accept():");
        //continue;
    }


    char *ip_string = inet_ntoa(con_addr.sin_addr);
    //printf("\nAccepted connection from %s",ip_string);
    syslog(LOG_DEBUG,"\nAccepted connection from %s",ip_string);


    datap = malloc(sizeof(slist_data_t));
    SLIST_INSERT_HEAD(&head,datap,entries);
    datap->threadParams.sock = newsock_t;
    pthread_create(&(datap->threadParams.threaddec),(void*)0,(void*)&Send_Receive,(void*)&(datap->threadParams));

    SLIST_FOREACH(datap,&head,entries){

        if(datap->threadParams.completion_status == false){

            continue;

        }

        else if (datap->threadParams.completion_status == true){

            pthread_join(datap->threadParams.threaddec,NULL);

        }

    }

}

closelog();
return 0;

}
