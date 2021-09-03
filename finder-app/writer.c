#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

int main(int argc,char **argv){

    openlog(" WRITER : ",LOG_PID,LOG_USER);

    int fd;
    int nr;

    char *filepath;
    char *str_data;

    char *path;

    char *temp_str;

    struct stat path_stat;

    if (argc < 3){

        syslog(LOG_ERR,"Few Arguments Specified\n");
        exit(-1);

    }

    if (argc > 3){

        syslog(LOG_ERR,"Extra Arguments Specified\n");
        exit(-1);

    }

    filepath = (char*)malloc(strlen(argv[1]));
    str_data = (char*)malloc(strlen(argv[2]));

    sscanf(argv[1],"%s",filepath);
    sscanf(argv[2],"%s",str_data);

    temp_str=(filepath+strlen(filepath)-1);

    int i=0;
    while (*(temp_str-i-1)!= '/'){
        i++;
    }
    i++;
    i++;

    path=(char*)malloc(strlen(filepath)-i);

    strncpy(path,filepath,strlen(filepath)-i);

    stat(path,&path_stat);
    if(!(path_stat.st_mode & S_IFDIR)){
        if (mkdir(path,S_IRWXU | S_IRWXG | S_IRWXO) == -1){
            printf("\nmkdir failed");
            return -1;
        }
    }

    if ( (fd=open(filepath,O_RDWR | O_TRUNC | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO)) == -1 ){

        //perror("Opening File:");
        syslog(LOG_ERR,"Error in opening file\n");
        return -1;
    }

    syslog(LOG_DEBUG,"Writing %s to file %s",str_data,filepath);

   if ( (nr=write(fd,str_data,strlen(str_data))) == -1){

       //perror("Writing File:");
       syslog(LOG_ERR,"Error in writing file\n");
       return -1;

   }

    free(filepath);
    free(str_data);
    free(path);

    return 0;
}