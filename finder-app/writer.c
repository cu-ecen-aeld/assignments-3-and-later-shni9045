#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>

int main(int argc,char **argv){

    openlog("Writer : ",LOG_PID,LOG_USER);

    int fd;
    int nr;

    char *filepath;
    char *str_data;

    char *path;

    char *temp_str;
    char *temp;

    char *command;

    struct stat path_stat={.st_mode=0};


    if (argc < 3){

        syslog(LOG_ERR,"Few Arguments Specified\n");
        exit(-1);

    }

    if (argc > 3){

        syslog(LOG_ERR,"Extra Arguments Specified\n");
        exit(-1);

    }

    filepath = (char*)malloc(sizeof(char)*(strlen(argv[1])+1));
    str_data = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));

    sscanf(argv[1],"%s",filepath);
    sscanf(argv[2],"%s",str_data);

    temp_str=(filepath+strlen(filepath)-1);
    temp=(filepath+strlen(filepath)-1);

    int i=0;

    while (*(temp_str-i-1)!= '/' && (i < (strlen(filepath)-2))){
        i++;
        temp--;
    }
    i++;
    i++;

   if((strlen(filepath)-i) > 0){


        path=(char*)malloc(sizeof(char)*(strlen(filepath)-i)+1);

        strncpy(path,filepath,strlen(filepath)-i);
        *(path+strlen(filepath)-i) = '\0';

        stat(path,&path_stat);
        if(!(path_stat.st_mode & S_IFDIR)){

            command=(char*)malloc(sizeof(char)*(10+strlen(path)));

            strncpy(command,"mkdir -p ",10);
            *(command+10) = '\0';
            strcat(command,path);

            system(command);

        }

        free(path);
        free(command);

    }


    if ( (fd=open(filepath,O_RDWR | O_TRUNC | O_CREAT,S_IRWXU )) == -1 ){

        //perror("Opening File:");
        syslog(LOG_ERR,"Error in opening file\n");
        return -1;
    }

    syslog(LOG_DEBUG,"Writing %s to file %s",str_data,temp);

   if ( (nr=write(fd,str_data,strlen(str_data))) == -1){

       //perror("Writing File:");
       syslog(LOG_ERR,"Error in writing file\n");
       return -1;

   }

    free(filepath);
    free(str_data);

    closelog();

    return 0;
}