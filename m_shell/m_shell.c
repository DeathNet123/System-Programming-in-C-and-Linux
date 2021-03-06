#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<fcntl.h>
#include <sys/types.h>
#include<constant.h> //this is my header
#include<m_shell_struct.h>
#include <sys/wait.h>
#include<regex.h>
#include<unistd.h>
#include<wait.h>

extern char **environ;
int logical_command = 0;

#define DEV

int handle_logical_command(char *command, regex_t *logic_preg, regex_t *pipe_preg);//this will handle the logical Operators in the given in put
/*Handle control is the main entry point and shell will always check for it */
int spawn_child(char *file, char **argv, int wait_flag, int in, int out); //this function will called when the child process has to be created and command is external one.
internal_command type_a(char * command); //this function will test weather the command is internal or external 
int handle_internal_command(char *command, internal_command meta_data, char **argv); //this function will called when the internal command is encountered
void clean_command(char *command); //this function will remove the extra white spaces and character such as \n, \r and more...
int handle_command_pipes(char *command, regex_t *pipe_preg);//function to handle the pipe command
int handle_command_generic(char *command, int wait_flag, int in, int out);//the is will be called to basic unit command..
void create_pipe(int **fd_pipes, int idx);

int main(int argc, char **argv)
{
    //compile the control command regex
    regex_t logic_preg;
    regex_t pipe_preg;
    regcomp(&logic_preg, "(.+)(&&|[|][|])(.+)", REG_EXTENDED);//regex for the logical command..
    regcomp(&pipe_preg, "(.+)[|](.+)", REG_EXTENDED);//regex for the pipe command..
    int status_dollar = 0; //this will hold the status of last command executed in $ variable..
    char command[MAX_CMD_LEN];//buffer for holding the command

    handle_command_generic("clear", 1, -1, -1); //this will cause the shell to clear the screen..
    
    while(1)
    {
        memset(command, '\0', MAX_CMD_LEN);
        #ifndef DEV
        printf("(%s@%s)-[%s]-$ ", getenv("LOGNAME"), getenv("NAME"), strcmp(getenv("HOME"), getenv("HOME"))?"~":getenv("PWD"));  
        #endif
        #ifdef DEV
        printf("\n(%s@%s)-$ ", getenv("LOGNAME"), getenv("NAME"));
        #endif
        fgets(command, MAX_CMD_LEN, stdin);
        clean_command(command);
    
        //status_dollar = handle_logical_command(strtok(command, "\n"), &logic_preg, &pipe_preg); //checking for the control commands if exist...
        if(logical_command)
        {
            logical_command = 0;
            continue;
        }
        status_dollar = handle_command_pipes(command, &pipe_preg);
    }

	return 0;
}

int handle_logical_command(char *command, regex_t *logic_preg, regex_t *pipe_preg)
{
    regmatch_t pmatch[CONTROL_COMMAND_GROUP];
    int test = regexec(logic_preg, command, CONTROL_COMMAND_GROUP, pmatch, 0);
    
    if(test) //check if command contains logical Operators or not if not return immediately..
    {
        return 0;
    } 
    logical_command = 1;

    //extracting the segments of the control command
    char left[pmatch[1].rm_eo - pmatch[1].rm_so + 1];
    char operator[3];
    char right[pmatch[3].rm_eo - pmatch[3].rm_so + 1];
    memset(left, '\0', sizeof(left));
    memset(right, '\0', sizeof(right));
    memset(operator, '\0', sizeof(operator));
    strncpy(left, command + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
    strncpy(operator, command + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
    strncpy(right, command + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
    
    #ifdef DEV
    printf("\nLEFT:%s OP:%s RIGHT:%s\n", left, operator, right);
    #endif

    //extracting the operator
    int op = 0;
    if(strcmp(operator, "&&") == 0) op = 1;
    else op = 2;
    
    int deep_well = regexec(logic_preg, left, CONTROL_COMMAND_GROUP, pmatch, 0); //checking weather we have to break down further or not...
    int last_status = handle_logical_command(left, logic_preg, pipe_preg);
    
    if(deep_well != 0)//if we don't have to break the command further will stop and handle the both left and right side and return..
    {
        switch(op) 
        {
            case 1:
                {
                    int right_status = 0;
                    int status = handle_command_pipes(left, pipe_preg);  
                    if(WIFEXITED(status) && status >> 8 != COM_NOT_EXIST) //if the left child exited normally then go fo rthe right thus satisfying the && logical Operator..
                    {
                        right_status = handle_command_pipes(right, pipe_preg);
                        if(WIFEXITED(right_status))
                            return 1;
                        else return 0;
                    }
                    else return 0;
                }
                break;
            case 2:
                int left_status = handle_command_pipes(left, pipe_preg);
                int right_status = handle_command_pipes(right, pipe_preg);
                return WIFEXITED(left_status) || WIFEXITED(right_status);
                break;
            default:
                printf("\nwe are broken somewhat!\n");
                exit(SABNORMAL); //right now this is explicit later will be replaced with the define one..
                break;
        }
    }
    else //this will occur once all of the stacks will be getting exhausted in reverse order..
    {
        switch (op)
        {
            case 1:
                if(last_status == 1)
                {
                    int status = handle_command_pipes(right, pipe_preg);
                    if(WIFEXITED(status))
                        return 1;
                    else return 0;
                } 
                else return 0;
                break;
            case 2:
                int status = handle_command_pipes(right, pipe_preg);
                return last_status && WIFEXITED(status);
                break;
            default:
                printf("\nTHERE WAS A PROBLEM IN RECURSION\n");
                exit(SABNORMAL);
                break;

        }
    }
}

int spawn_child(char *file, char **argv, int wait_flag, int in, int out)
{

    int cpid = fork();
    if(cpid == 0)
    {
        
        if(in != - 1) dup2(in, 0);
        if(out != - 1) dup2(out, 1);

        int rv = execvp(file, argv);
        if(rv != 0)
        {
            printf("No such command found: %s\n", file);
            return COMMAND_NOT_FOUND;
        }
    }
    else
    {
        if(wait_flag)
        {
            int status = 0;
            wait(&status);
            return status;
        }
        return 1;
    }
}

void clean_command(char *command) // the purpose of this function is to remove the characters such as \n and \r etc and extra white spaces
{
    char cmd[1024];
    int count = 0;
    
    memset(cmd, '\0', 1024);
    strcpy(cmd, command);
    memset(command, '\0', 1024);
    char prev = cmd[0];
    
    for(int idx = 0; idx < 1024; idx++)
    {
        if(cmd[idx] == '~'||cmd[idx] == ' ' ||(cmd[idx] >= 'a' && cmd[idx] <= 'z') || (cmd[idx] >= 'A' && cmd[idx] == 'Z') || (cmd[idx] >= '0' && cmd[idx] <= '9') || cmd[idx] == '-' || cmd[idx] == '&' || cmd[idx] == '|') 
        {
            if(prev == ' ' && cmd[idx] == ' ') continue;
            command[count++] = cmd[idx];
            prev = cmd[idx];
        }
    }
}

int handle_command_pipes(char *command, regex_t *pipe_preg)
{
    regmatch_t pmatch[PIPE_CMD_GROUP];
    int count_pipes = 0; //this will decide how many pipes are needed to be created..
    int **fd_pipes;//will hold the file descriptor for each pipes
    char *cmd = NULL;

    int test = regexec(pipe_preg, command, PIPE_CMD_GROUP, pmatch, 0);
    if(test)
    {
        return handle_command_generic(command, 1, -1, -1); //if there is no pipe leave and move to the handle generic
    }
    //count the number of pipes
    for(int idx = 0; command[idx] != '\0'; idx++)
    {
        if(command[idx] == '|')
            count_pipes++;
    }
    //allocate the array of pipes_fd
    fd_pipes = (int **) malloc(count_pipes * sizeof(int *));
    for(int idx = 0; idx < count_pipes; idx++)
    {
        fd_pipes[idx] = (int *) malloc(sizeof(int) * 2);
    }

    //sending the segments to command handler...
    create_pipe(fd_pipes, 0);
    strtok(command, "|");
    handle_command_generic(command, 0, -1, fd_pipes[0][1]);
    
    for(int idx = 1; idx < count_pipes; idx++)
    {
        cmd = strtok(NULL, "|");
        printf("%s", cmd);
        create_pipe(fd_pipes, idx);
        handle_command_generic(cmd, 0, fd_pipes[idx - 1][0], fd_pipes[idx][1]);
    }
    cmd = strtok(NULL, "|");
    int last_status = handle_command_generic(cmd, 1, fd_pipes[count_pipes - 1][0], -1);
    
    for(int idx = 0; idx < count_pipes; idx++) {wait(NULL);}
    
    if(WIFEXITED(last_status))
        return 1;
    
    return 0;   
}

int handle_command_generic(char *command, int wait_flag, int in, int out)
{
    char *argv[ARG_NUM];
    argv[0] = command;
    argv[1] = NULL;
    
    int rv = spawn_child(command, argv, wait_flag, in, out);

    if(in != -1) close(in);
    if(out != -1) close(out);
    
    return rv;
}

void create_pipe(int **fd_pipes, int idx)
{
    int rv_pipe = pipe(fd_pipes[idx]);
    if(rv_pipe == 1)
    {
            perror("pipe");
            exit(EXIT_FAILURE);
    }
}