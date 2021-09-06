/*
* Code for AESD Assignment 2 for writer.sh script functionality in C language
* @shrikant nimhan - shni9045
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>

int main(int argc,char **argv){

    // Setup syslog with LOG_USER facility
    openlog("Writer : ",LOG_PID,LOG_USER);

    // Variables to check return value of file operations
    int fd;
    int nr;

    char *filepath;                    // Stores user given file path
    char *str_data;                    // Stores user given string to write

    char *path;                        // Stores directory path 


    // Temporary variables to operate on file path strings
    char *temp_str;
    char *temp;

    char *command;

    // Stat type strcuture to check for existence of directory through stat() call
    struct stat path_stat={.st_mode=0};

    // Error check for incorrect number of  paramters specified by user
    if (argc < 3){

        syslog(LOG_ERR,"ERROR:Few Arguments Specified\n");
        return 1;

    }

    if (argc > 3){

        syslog(LOG_ERR,"ERROR:Extra Arguments Specified\n");
        return 1;

    }

    // Allocate memory to store path & string with null characters
    filepath = (char*)malloc(sizeof(char)*(strlen(argv[1])+1));
    str_data = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));

    sscanf(argv[1],"%s",filepath);
    sscanf(argv[2],"%s",str_data);

    // Initialize pointers to strings
    temp_str=(filepath+strlen(filepath)-1);
    temp=(filepath+strlen(filepath)-1);

    int i=0;

    // Extract path by removing filename from string
    while (*(temp_str-i-1)!= '/' && (i < (strlen(filepath)-2))){
        i++;
        temp--;
    }
    i++;
    i++;

   // Check for existence of directory and if not created one
   if((strlen(filepath)-i) > 0){


        path=(char*)malloc(sizeof(char)*(strlen(filepath)-i)+1);

        strncpy(path,filepath,strlen(filepath)-i);
        *(path+strlen(filepath)-i) = '\0';

        // stat() function call to store information about directory in stat structure
        stat(path,&path_stat);

        // check for directory existence with mode field of structure
        if(!(path_stat.st_mode & S_IFDIR)){

            command=(char*)malloc(sizeof(char)*(10+strlen(path)));

            strncpy(command,"mkdir -p ",10);
            *(command+10) = '\0';
            strcat(command,path);

            // Create directory with mkdir command
            system(command);

        }

        // free temporary path and command strings
        free(path);
        free(command);

    }

    // open() system call to open an existing file or optionally create one if it does not exist

    if ( (fd=open(filepath,O_RDWR | O_TRUNC | O_CREAT,S_IRWXU )) == -1 ){

        syslog(LOG_ERR,"Error in opening/creating file\n");
        return -1;
    }
    
    // Log DEBUG message
    syslog(LOG_DEBUG,"Writing %s to file %s",str_data,temp);

    // write() system call to  write the user specified string 
   if ( (nr=write(fd,str_data,strlen(str_data))) == -1){

       syslog(LOG_ERR,"Error in writing to file\n");
       return -1;

   }

    // Free Allocated string pointers for filename & string
    free(filepath);
    free(str_data);

    // Close logging
    closelog();

    return 0;
}