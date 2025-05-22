#define N_AUTO 4

#include "incrocio.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>


void incrocio(int* pipe_g_i);

void garage(int* pipe_g_i);

int main(){
    pid_t pid = fork();
    
    int pipe_g_i[2]; //la pipe che gestisce la comunicazione dal garage all'incrocio

    int pipeRet = pipe(pipe_g_i);

    if (pipeRet == -1)
    {
        perror("Errore in creazione della prima pipe\n");
        exit(EXIT_FAILURE);
    }   


    if (pid == 0)
    {
        //incrocio(pipe_g_i);
        close(pipe_g_i[1]);
        char buf[255];
        memset(buf, '\0', sizeof(buf));
        
        int rRet = read(pipe_g_i[0], buf, sizeof(buf));
        
        if (rRet < 0)
        {
            printf("Incrocio: errore in read\n");
        }
        
        
        printf("Incrocio: messaggio ricevuto: %s\n", buf);
    }else{
        //garage(pipe_g_i);
        close(pipe_g_i[0]);
        char buf[255];
        strcpy(buf, "prova comunicazione");
        printf("Garage: invio messaggio ''%s''\n", buf);
        int wRet = write(pipe_g_i[1], buf, strlen(buf));
        if (wRet < 0)
        {
            perror("Garage: Errore in write\n");
        }
        
    }

    return EXIT_SUCCESS;
}

void incrocio(int* pipe_g_i){
    close(pipe_g_i[1]);
    char buf[255];
    memset(buf, '\0', sizeof(buf));
    
    read(pipe_g_i[0], buf, sizeof(buf));
    printf("Incrocio: messaggio ricevuto: %s\n", buf);
    return;
}


void garage(int* pipe_g_i){
    close(pipe_g_i[0]);
    char* buf = "Prova comunicazione\0";
    printf("Garage: invio messaggio ''%s''\n", buf);
    write(pipe_g_i[1], buf, strlen(buf));
    return;
}