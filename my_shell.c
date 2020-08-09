#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "my_shell.h"
#define BUFFER 80
// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise

void catch_chld(int sig_chld)
{
    signal(SIGCHLD,catch_chld);
	while (waitpid(-1,NULL,WNOHANG)>0){
        continue;
	}
}

void catch_sigint(int sig_int){
    signal(SIGINT,SIG_IGN);
}

void catch_chld_for_child(int sig_chld){
	signal(SIGINT, catch_chld);
	kill(getpid(),SIGSTOP);
}

int check_pipe(char** arglist,int count){
    int mone = 0;
    while (mone < count){
        if (**(arglist+mone)=='|')
            return mone;
        mone++;
    }
    return 0;
}

void do_pipe(char**arglist,int count,int index){
    char**n_arglist = arglist+index +1;
    *(arglist+index)=NULL;
    pid_t child1,child2;
    int pipe_descs[2];
    if (pipe(pipe_descs) < 0) {
        fprintf(stderr, "there is problem to open pipe\n");
        exit(1);
    }
    child1 = fork();
    if (child1<0)
    {
        fprintf(stderr,"there is no way to start new process now");
        exit(1);
    }
    if (child1 == 0) {
        signal(SIGINT,catch_chld_for_child);
        close(pipe_descs[0]);
        dup2(pipe_descs[1],STDOUT_FILENO);
        close(pipe_descs[1]);
        execvp(arglist[0],arglist);
    }
    else {
        child2 = fork();
        if (child2<0){
            fprintf(stderr,"there is no way to start new process now");
            exit(1);
        }
        if (child2==0){
            signal(SIGINT,catch_chld_for_child);
            close(pipe_descs[1]);
            dup2(pipe_descs[0],STDIN_FILENO);
            close(pipe_descs[0]);
            execvp(n_arglist[0],n_arglist);
        }
        else{
            wait(NULL);
            wait(NULL);
        }
    }
}

void do_background(char** arglist , int count){
    *(arglist+count-1)=NULL;
    //printf("%c",**(arglist+count-2));
    pid_t child = fork();
    if (child<0){
        fprintf(stderr,"there is no way to start new process now");
        exit(1);
    }
    if (child == 0){
        signal(SIGCHLD, catch_sigint);
        execvp(arglist[0],arglist);
    }
    else {
        return;
    }
}

int process_arglist(int count, char** arglist) {

    signal(SIGINT,catch_sigint);
    if (**(arglist + count - 1) == '&') {

        do_background(arglist,count);
        return 1;
    }
    int isPipe = check_pipe(arglist,count);
    if(isPipe){
        do_pipe(arglist,count,isPipe);
        return 1;
    }
    pid_t child = fork();
    if (child<0){
        fprintf(stderr,"there is no way to start new process now");
        exit(1);
    }
    if(child==0){
        signal(SIGINT,catch_chld_for_child);
        execvp(arglist[0],arglist);
    }
    else{
        wait(NULL);
    }
    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
    signal(SIGINT,catch_sigint);
    puts("hello wellcom to our shell");
    return  0 ;
}

int finalize(void)
{
    printf("buy buy \n");
    return 0;
}
