    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <errno.h>
    #include <signal.h>
    #include <syslog.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    

    #define BUFSIZE 100

    #define PORT "9000"
    #define BACKLOG 2

    char* read_buf;

    char ip_string[INET6_ADDRSTRLEN];

    int check;
    int sock_t;
    int newsock_t;

    int fd;

    int num_bytes;

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


    }

    int main()
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


        while(1) {

        check = listen(sock_t, BACKLOG);

        if (check == -1){

            perror("\nERROR listen():");
            exit(-1);

        }

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



        check = fork();
        if (check < 0){

            perror("\nERROR fork():");
            exit(-1);
        }


        if(!check){

            char *ch;
            ssize_t wbytes;

            read_buf = (char*)malloc(sizeof(char)*BUFSIZE);



            fd = open("/var/tmp/aesdsocketdata",O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
            if(fd<0){
                perror("\nERROR open():");
                exit(-1);
            }
            
            // Read till packet is complete
            while(1){

            num_bytes = recv(newsock_t,read_buf, BUFSIZE-1, 0);
            if(num_bytes == -1){
                perror("ERROR recv():");
                exit(-1);     
            }

            ch = strchr(read_buf,'\n');

            if(ch != NULL){

                read_buf[num_bytes]='\0';

                // Block signals to avoid partial write
                if (sigprocmask(SIG_BLOCK,&set,NULL) == -1){
                    perror("\nERROR sigprocmask():");
                    exit(-1);
                }

                wbytes = write(fd,read_buf,strlen(read_buf));
                if (wbytes == -1){
                    perror("\nERROR write():");
                    exit(-1);
                }

                // Unblock signals to avoid partial write
                if (sigprocmask(SIG_UNBLOCK,&set,NULL) == -1){
                    perror("\nERROR sigprocmask():");
                    exit(-1);
                }

                
                // Send packet 
                if (send(newsock_t,read_buf,sizeof(read_buf), 0) == -1){
                perror("send");
                printf("\n Failure");
                }

                //close(newsock_t);
                //free(read_buf);
                break;

            }

        }


        close(newsock_t);
    }

}

    closelog();
    return 0;
}