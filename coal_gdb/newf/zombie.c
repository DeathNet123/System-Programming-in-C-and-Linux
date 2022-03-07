#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
int main(void)
{ 
    int cpid = fork(); 
    if (cpid == 0)
    { 
        printf("Terminating child with PID = %ld\n", 
        (long)getpid()); 
        exit (0); 
    } 
    else{ 
        printf("Running parent, PID=%ld\n",(long)getpid()); 
        wait(NULL);
        while(1); 
    } 
    return 0;
}
