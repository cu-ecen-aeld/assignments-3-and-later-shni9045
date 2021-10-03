
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


#define BUFSIZE 100

#define PORT "9000"
#define BACKLOG 2


struct stat  stat_data;
int status;

char *read_buf,*write_buf,*temp_buf,*out_buf;

char ip_string[INET6_ADDRSTRLEN];

pid_t check;
int sock_t;
int newsock_t;

int fd;

int buff_loc=0;
int currbuf_size=BUFSIZE;

int num_bytes;
int total_bytes=0;

ssize_t read_bytes;

struct addrinfo host;
struct addrinfo *servinfo;

struct sockaddr_storage con_addr;

struct sockaddr *temp_addr;



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

    closelog();

    free(read_buf);
    free(write_buf);

    exit(0);


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
        exit(-1);

    }

    sock_t = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if(sock_t == -1) {

        perror("\nERROR socket():");
        exit(-1);

    }

    check = bind(sock_t,servinfo->ai_addr, servinfo->ai_addrlen);

    if (check == -1){

        perror("\nERROR bind():");
        freeaddrinfo(servinfo);
        exit(-1);

    }

    // Setup signals

    if(signal(SIGINT,sig_handler) == SIG_ERR){

        syslog(LOG_ERR,"\nError in setting signals");
        exit(-1);

    }

    if(signal(SIGTERM,sig_handler) == SIG_ERR){

        syslog(LOG_ERR,"\nError in setting signals");
        exit(-1);

    }

    // Setup signal mask
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    sigaddset(&set,SIGTERM);

    freeaddrinfo(servinfo);

    check = listen(sock_t, BACKLOG);

    if (check == -1){

        perror("\nERROR listen():");
        exit(-1);

    }


    read_buf = (char*)malloc(sizeof(char)*BUFSIZE);
    write_buf = (char*)malloc(sizeof(char)*BUFSIZE);

    if (argc == 2){

        if (!strcmp("-d",argv[1])){

        check = fork();
        if (check == -1){

            perror("\nERROR fork():");
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


    newsock_t = accept(sock_t,(struct sockaddr *)&con_addr,&len);

    if (newsock_t == -1 ){
        perror("\nERROR accept():");
        continue;
        }

    temp_addr = (struct sockaddr *)&con_addr;

    if (temp_addr->sa_family == AF_INET){

        struct in_addr* con_ip;
        con_ip = &(((struct sockaddr_in*)servinfo)->sin_addr);
        inet_ntop(servinfo->ai_family, con_ip,ip_string, sizeof(ip_string));
            
        printf("\nAccepted connection from %s",ip_string);

    }

    else {
            
        struct in6_addr* con_ip;
        con_ip = &(((struct sockaddr_in6*)servinfo)->sin6_addr);
        inet_ntop(servinfo->ai_family, con_ip,ip_string, sizeof(ip_string));
        
        printf("\nAccepted connection from %s",ip_string);

    }


        char *ch;
        ssize_t wbytes;

        buff_loc=0;

        // Read till packet is complete
        while((num_bytes = recv(newsock_t,read_buf+buff_loc, BUFSIZE, 0))>0){

        if(num_bytes == -1){
            perror("ERROR recv():");
            exit(-1);    

        }

        buff_loc += num_bytes;

        if (buff_loc >= currbuf_size){

        currbuf_size+=BUFSIZE;

        temp_buf = realloc(read_buf,sizeof(char)*currbuf_size);

        if(temp_buf == NULL){
            syslog(LOG_ERR,"\nErrror in realloc");
        }

        else read_buf = temp_buf;
        
        }

        ch = strchr(read_buf,'\n');

        if(ch != NULL) break;
       
        
        }

        // Block signals to avoid partial write
        if (sigprocmask(SIG_BLOCK,&set,NULL) == -1){
            perror("\nERROR sigprocmask():");
            exit(-1);
        }


        fd = open("/var/tmp/aesdsocketdata",O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
        if(fd<0){
            perror("\nERROR open():");
            exit(-1);
        }


        wbytes = write(fd,read_buf,buff_loc);
        if (wbytes == -1){
            perror("\nERROR write():");
            exit(-1);
        }

        total_bytes+=wbytes;

        status = stat("/var/tmp/aesdsocketdata",&stat_data);
        int si;
        if (status == 0){
        si=stat_data.st_size;
        printf("hhhh%d",si);
        }

        out_buf=realloc(write_buf,sizeof(char)*si);
        write_buf=out_buf;


        lseek(fd,0,SEEK_SET);

        read_bytes = read(fd,write_buf,si);
        printf("\nsss%d",total_bytes);
        printf("\nfff:%s",write_buf);
        if (send(newsock_t,write_buf,read_bytes, 0) == -1){
        perror("send");
        printf("\n Failure");
        }

        // Unblock signals to avoid partial write
        if (sigprocmask(SIG_UNBLOCK,&set,NULL) == -1){
            perror("\nERROR sigprocmask():");
            exit(-1);
        }

       close(newsock_t);

}

closelog();
return 0;
}