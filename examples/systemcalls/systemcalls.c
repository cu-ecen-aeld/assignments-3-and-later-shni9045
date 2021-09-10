#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/  
    int sys_return;

    if(cmd==NULL){

        printf("\nEmpty Command");
        //return false;
    }

    sys_return=system(cmd);


    if(sys_return == -1){

        printf("\nError in invocation of system() call");
        perror("\nERROR system():");

        return false;

    }

    else {

        if (WIFEXITED(sys_return)){

            if(WEXITSTATUS(sys_return) !=0 ){
                printf("\nNon-Zero Value returned by Issued Command");
                return false;
            }

        }

    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    //char **temp_arg;
    pid_t pid;
    int status;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/
    pid=fork();

    if(pid == -1){

        perror("ERROR fork() Call:");
        return false;

    }

    else if(pid==0){

        //temp_arg=command+1;
        //execl(command[0],*temp_arg,(char *) NULL);
        //execv(command[0],*temp_arg);
        execv(command[0],command);

        return false;

    }

    if(waitpid(pid,&status,0) == -1){
        
        perror("ERROR waitpid() call:");
        return false;

    }

    else if (WIFEXITED(status)){

    if(WEXITSTATUS(status) !=0 ){
            printf("\nNon-Zero Value returned by Issued Command");
            return false;
        }

    }


    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    pid_t pid;
    int status;
    //int childpid;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    //char **temp_arg;
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/  
    /*
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU);

    if (fd < 0) 
    {
        perror("ERROR open() call:"); 
        return false;
    }

    switch (childpid = fork()) {

        case -1: {
            perror("ERROR fork() call:");
            return false;
            }
        case 0:{
            if (dup2(fd, 1) < 0) {
                perror("ERROR dup2() call:"); 
                return false; 
            }
            //close(fd);
            //temp_arg=command+1;
            //execl(command[0],*temp_arg,(char *) NULL);
            //execv(command[0],*temp_arg);
            execv(command[0],command);
            perror("execvp:"); 
            return false;
        }
        //default:
            //close(fd);

    }*/

    int filefd = open(outputfile, O_WRONLY|O_CREAT, 0666);
    if (!(pid=fork())) {
    close(1);//Close stdout
    dup(filefd);
    execv(command[0],command);
    perror("execvp:");
   
    } else {
    close(filefd);
    return false;
    //wait(NULL);
    }

    if(waitpid(pid,&status,0) == -1){
        
        perror("ERROR waitpid() call:");
        return false;

    }

    else if (WIFEXITED(status)){

    if(WEXITSTATUS(status) ==-1 ){
            printf("\nNon-Zero Value returned by Issued Command");
            return false;
        }

    }

    va_end(args);
    
    return true;
}
