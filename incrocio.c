#include "incrocio.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void SigTermHandlerIncrocio();

void SigTermHandlerGarage();

void SigTermHandlerAuto() { //non dovrebbe servire, per sicurezza lo metto
    printf("Automobile: Ricevuto SIGTERM\n");
    return;
}

void incrocio(int *pipe_g_i); // come argomento la pipe dal garage all'incrocio

void garage(int *pipe_g_i); //pipe in direzione opposta

void automobile(int numAuto, int numStrada);

int initFIFO();

char* getPipePath(int i, char* name);

volatile sig_atomic_t keep_going = 1;

pid_t garage_pid;

int main()
{
    garage_pid = -1;
    if(initFIFO() == -1){
        perror("Errore in creazione delle named pipe\n");
        exit(EXIT_FAILURE);
    }

    int pipe_g_i[2]; // array con le due "pipe end"

    int pipeRet = pipe(pipe_g_i); // creazione dell'effettiva pipe

    if (pipeRet == -1)
    {
        perror("Errore in creazione della prima pipe\n");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork(); 

    if (pid == 0) // processo figlio
    {
        garage(pipe_g_i);       
        printf("Garage: processo finito\n"); 
        exit(EXIT_SUCCESS);
    }
    else // processo padre
    {
        printf("PID incrocio: %d\n", getpid());
        garage_pid = pid;
        incrocio(pipe_g_i);
    }

    printf("Tutti i processi terminati\n");

    exit(EXIT_SUCCESS);
}


void incrocio(int *pipe_g_i)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = SigTermHandlerIncrocio;
    sa.sa_flags = 0; //disabilita il sa_restart
    sigaction(SIGTERM, &sa, NULL);
    int pipefd_out[NUM_AUTO]; //array per la comunicazione tra incrocio e automobili
    int pipefd_in[NUM_AUTO]; //

    close(pipe_g_i[1]); // chiudo la pipe end di scrittura dovendo solo leggere
    int arrAuto[NUM_AUTO];


    int incrocioFD = open("incrocio.txt", O_WRONLY|O_CREAT|O_APPEND, S_IWUSR|S_IRUSR); //apertura del file incrocio.txt
    if (incrocioFD == -1)
    {
        perror("Errore in apertura del file incrocio.txt\n");
        exit(EXIT_FAILURE);
    }

    while(keep_going){
        int rRet = read(pipe_g_i[0], arrAuto, sizeof(arrAuto));
        if (rRet == -1)
        {
            if(errno == EINTR){ //lettura interrotta da sigterm (in generale, da un segnale)
                continue;
            } 
            perror("Incrocio: errore in lettura da garage\n");
            exit(EXIT_FAILURE);
        }


        //printf("Apertura delle named pipe per incrocio\n");
        for (int i = 0; i < NUM_AUTO; i++) //crea un array con le pipe per ogni auto
        {
            char* outputPath = getPipePath(i, "i_a_pipe"); //la funzione getPipePath restituisce il path della fifo dato nome e indice
            char* inputPath = getPipePath(i, "a_i_pipe");
            //printf("Path output pipe %d: %s\n", i, outputPath); //printf di debug
            //printf("Path input pipe %d: %s\n", i,  inputPath);
            pipefd_out[i] = open(outputPath, O_WRONLY); //in output
            pipefd_in[i] = open(inputPath, O_RDONLY); //in input 
        }
        //printf("pipe per incrocio aperte\n");


        int wRet;
        for (int i = 0; i < NUM_AUTO; i++)
        {
            int next = GetNextCar(arrAuto);
            printf("Incrocio: %d° auto ad attraversare: %d\n", i+1, next);
            int msg = PROCEDI;
            wRet = write(pipefd_out[next], &msg, sizeof(msg));
            if(wRet == -1){
                perror("Incrocio: errore in scrittura\n");
                exit(EXIT_FAILURE);
            }

            int aRet; //messaggio di ritorno dall'automobile

            int rRet = read(pipefd_in[next], &aRet, sizeof(aRet));
            if (rRet == -1)
            {
                perror("Incrocio: errore in lettura dall'automobile\n");
                exit(EXIT_FAILURE);
            }
            if (aRet == AUTO_TRANSITATA)
            {
                printf("Incrocio: conferma ricevuta che l'auto %d è transitata\n", next);
                arrAuto[next] = -1; //si segna l'auto come transitata
                char line[3] = {next+'0', '\n', '\0'};
                int wRet = write(incrocioFD, line, strlen(line));
                if (wRet == -1)
                {
                    perror("Errore in scrittura su incrocioFD\n");
                    exit(EXIT_FAILURE);
                }

            }else{
                perror("Incrocio: Errore di lettura dall'automobile\n");
                exit(EXIT_FAILURE);
            }
        }
        
    }
    kill(garage_pid, SIGTERM);
    close(incrocioFD);
    for (int i = 0; i < NUM_AUTO; i++)
    {
        close(pipefd_in[i]);
        close(pipefd_out[i]);
    }
    close(pipe_g_i[0]);
    int status;
    waitpid(garage_pid, &status, 0);
    return;
}

void garage(int *pipe_g_i)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = SigTermHandlerGarage;
    sigaction(SIGTERM, &sa, NULL);


    close(pipe_g_i[0]);

    int automobili[NUM_AUTO];

    while (keep_going){
        
        for (int i = 0; i < NUM_AUTO; i++)
        {
            automobili[i] = EstraiDirezione(i);
            printf("Automobile numero %d, strada %d\n", i, automobili[i]);
        }     
        int wRet = write(pipe_g_i[1], automobili, sizeof(automobili));

        if (wRet == -1)
        {
            perror("Garage: Errore in write\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < NUM_AUTO; i++)
        {


            int pid = fork();
            if (pid == -1)
            {
                printf("Garage: errore in fork\n");
                exit(EXIT_FAILURE);
            }

            if (pid == 0)
            {
                automobile(i, automobili[i]);
                exit(EXIT_SUCCESS);
            }
        }

        int status;
        while (wait(&status) > 0); //attende che finiscano tutti i processi figli
        sleep(1);
    }

    close(pipe_g_i[1]);
    
    return;
}

void automobile(int numAuto, int numStrada){
    //printf("Sono l'automobile %d e devo prendere la strada %d\n", numAuto, numStrada);
    
    char* i_path = getPipePath(numAuto, "i_a_pipe");
    char* o_path = getPipePath(numAuto, "a_i_pipe");
    
    
    //printf("Automobile %d: apertura pipe input %s\n", numAuto, i_path);
    int pipe_in = open(i_path, O_RDONLY); //pipe dall'incrocio alle automobili
    if (pipe_in == -1)
    {
        perror("Automobile: errore in apertura della pipe dall'incrocio\n");
        exit(EXIT_FAILURE);
    }
    int pipe_out = open(o_path, O_WRONLY);
    if (pipe_out == -1)
    {
        perror("Automobile: errore in apertura della pipe per l'incrocio\n");
        exit(EXIT_FAILURE);
    }
    int msg;

    int rRet = read(pipe_in, &msg, sizeof(msg));

    if (rRet == -1)
    {
        printf("Automobile: errore in lettura\n");
        printf("Errore: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    
    int autoFd = open("auto.txt", O_CREAT|O_WRONLY|O_APPEND, S_IWUSR|S_IRUSR);
    if (autoFd == -1)
    {
        perror("Automobile: errore in apertura del file auto.txt\n");
        exit(EXIT_FAILURE);
    }
    


    if (msg == PROCEDI)
    {
        printf("Automobile %d: procedo nell'attraversamento\n", numAuto);
        char line[3];
        
        line[0] = numAuto+'0';
        line[1] = '\n';
        line[2] = '\0';

        int wfRet = write(autoFd, line, strlen(line));
        if (wfRet == -1)
        {
            perror("Automobile: errore in scrittura sul file\n");
            exit(EXIT_FAILURE);
        }
        close(autoFd);
        int msg = AUTO_TRANSITATA;

        int wpRet = write(pipe_out, &msg, sizeof(msg));

        if (wpRet == -1)
        {
            perror("Automobile: errore in scrittura su pipe\n");
            exit(EXIT_FAILURE);
        }
        

    }else{
        printf("Automobile: messaggio ricevuto non conforme\n");
        printf("Messaggio: %d\n", msg);
        exit(EXIT_FAILURE);
    }
    close(pipe_in);
    close(pipe_out);
    return;
}

int initFIFO(){
    
    for (int i = 0; i < NUM_AUTO; i++)
    {
        char* i_a_path = getPipePath(i, "i_a_pipe");
        char* a_i_path = getPipePath(i, "a_i_pipe");


        int fRet;

        if (access(i_a_path, F_OK) == -1) //access ritorna -1 se il file non esiste
        {
            printf("Creo pipe %s\n", i_a_path);
            fRet = mkfifo(i_a_path, S_IRUSR|S_IWUSR); 
            if (fRet == -1)
            {
                return -1; //ritorna -1 SOLO SE mkfifo ha dato errore, altrimenti continua
            }
        }

        if (access(a_i_path, F_OK) == -1) //access ritorna -1 se il file non esiste
        {
            printf("Creo pipe %s\n", a_i_path);
            fRet = mkfifo(a_i_path, S_IRUSR|S_IWUSR); 
            if (fRet == -1)
            {
                return -1; //ritorna -1 SOLO SE mkfifo ha dato errore, altrimenti continua
            }
        }
    }
    return 0;
}


char* getPipePath(int i, char* name){
    char ind[2];
    char* path = malloc(strlen(name) + 7); //lunghezza del nome + lunghezza di "./tmp/" + 1 (\0)
    strcpy(path, "./tmp/");
    strcat(path, name);//metodo non molto elegante per avere i nomi delle pipe
    ind[0] = i+'0'; //con indice + '0' si ottiene il carattere ASCII dell'indice 
    ind[1] = '\0';
    strcat(path, ind);
    return path;
}

void SigTermHandlerIncrocio(){
    printf("Incrocio: ricevuto sigterm\n");
    keep_going = 0;
    return;
}

void SigTermHandlerGarage()
{
    printf("Garage: ricevuto sigterm\n");
    keep_going = 0;
    return;
}