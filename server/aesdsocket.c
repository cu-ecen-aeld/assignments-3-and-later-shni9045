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

// macro definition for  buffer size
#define BUFSIZE 500
// macro definiton for port#
#define PORT "9000"
// macro definition for pending connections
#define BACKLOG 2

// structure to store file metadata
struct stat  stat_data;
int status;

// Buffer declarations
char *read_buf,*write_buf,*temp_buf,*out_buf;


pid_t check;
int sock_t;
int newsock_t;

int fd;

int buff_pos=0;
int currbuf_size=BUFSIZE;

int num_bytes;

ssize_t read_bytes;

struct addrinfo host;
struct addrinfo *servinfo;

struct sockaddr_in con_addr;

static void sig_handler(int signo){


    // close listening server socket fd
    close(sock_t);

    close(newsock_t);

    // close file descriptor
    close(fd);

    // Delete file
    if(remove("/var/tmp/aesdsocketdata") != 0){
        syslog(LOG_ERR,"\nERROR in deleting file");
    }
    
    // close log
    closelog();
    
    // free pointers
    free(read_buf);
    free(write_buf);
    
    exit(0);


}

void close_graceful(){

      // close listening server socket fd
    close(sock_t);

    close(newsock_t);

    // close file descriptor
    close(fd);

    // Delete file
    if(remove("/var/tmp/aesdsocketdata") != 0){
        syslog(LOG_ERR,"\nERROR in deleting file");
    }
    
    // close log
    closelog();
    
    // free pointers
    free(read_buf);
    free(write_buf);
    


}

int main(int argc, char* argv[])
{


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

    // allocate memory for buffers
    read_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    write_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    
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
    

    char *ch;
    ssize_t wbytes;

    buff_pos=0;

    // Read till packet is complete
    while((num_bytes = recv(newsock_t,read_buf+buff_pos, BUFSIZE, 0))>0){

    if(num_bytes == -1){
        perror("ERROR recv():");
        close_graceful();
        exit(-1);    

    }

    buff_pos += num_bytes;
    
    // dynamicall increase buffer size for incoming packets
    if (buff_pos >= currbuf_size){

    currbuf_size+=BUFSIZE;

    temp_buf = realloc(read_buf,sizeof(char)*currbuf_size);

    if(temp_buf == NULL){
        syslog(LOG_ERR,"\nErrror in realloc");
        close_graceful();
        exit(-1);
    }

    else read_buf = temp_buf;
    
    }
    
    // Search for NULL character
    ch = strchr(read_buf,'\n');

    if(ch != NULL) break;
    
    
    }


    // Block signals to avoid partial write
    if (sigprocmask(SIG_BLOCK,&set,NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }

    // Open file for writing
    fd = open("/var/tmp/aesdsocketdata",O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
    if(fd<0){
        perror("\nERROR open():");
        close_graceful();
        exit(-1);
    }

    // Write to file
    wbytes = write(fd,read_buf,buff_pos);
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

    out_buf=realloc(write_buf,sizeof(char)*si);
    write_buf=out_buf;


    lseek(fd,0,SEEK_SET);

    read_bytes = read(fd,write_buf,si);
    
    // Send packets
    if (send(newsock_t,write_buf,read_bytes, 0) == -1){
    perror("send");
    close_graceful();
    }

    // Unblock signals to avoid partial write
    if (sigprocmask(SIG_UNBLOCK,&set,NULL) == -1){
        perror("\nERROR sigprocmask():");
        close_graceful();
        exit(-1);
    }

    close(newsock_t);

}

closelog();
return 0;

}