#include "incrocio.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void incrocio(int *pipe_g_i); // come argomento la pipe dal garage all'incrocio

void garage(int *pipe_g_i); //pipe in direzione opposta

void automobile(int numAuto, int numStrada);

int initFIFO();

char* getPipePath(int i, char* name);


int main()
{

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
    pid_t pid = fork(); // fork dei processi

    if (pid == 0) // processo figlio
    {
        incrocio(pipe_g_i);
    }
    else // processo padre
    {
        garage(pipe_g_i);
    }

    return EXIT_SUCCESS;
}

void incrocio(int *pipe_g_i)
{
    close(pipe_g_i[1]); // chiudo la pipe end di scrittura dovendo solo leggere
    int arrAuto[NUM_AUTO];
    int rRet = read(pipe_g_i[0], arrAuto, sizeof(arrAuto));
    if (rRet == -1)
    {
        perror("Incrocio: errore in lettura da garage\n");
        exit(EXIT_FAILURE);
    }

    int pipefd_out[4];
    int pipefd_in[4];

    //printf("Apertura delle named pipe per incrocio\n");
    for (int i = 0; i < NUM_AUTO; i++) //crea un array con le pipe per ogni auto
    {
        char* outputPath = getPipePath(i, "i_a_pipe");
        char* inputPath = getPipePath(i, "a_i_pipe");
        //printf("Path output pipe %d: %s\n", i, outputPath);
        //printf("Path input pipe %d: %s\n", i,  inputPath);
        pipefd_out[i] = open(outputPath, O_WRONLY); //in output
        pipefd_in[i] = open(inputPath, O_RDONLY); //in input 
    }
    //printf("pipe per incrocio aperte\n");


    int wRet;
    for (int i = 0; i < NUM_AUTO; i++)
    {
        int next = GetNextCar(arrAuto);
        arrAuto[next] = -1;
        printf("Incrocio: %d° auto ad attraversare: %d\n", i+1, next);
        char msg = PROCEDI;
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
        }else{
            perror("Errore da aRet\n");
            exit(EXIT_FAILURE);
        }

    }
    
    
    

    return;
}

void garage(int *pipe_g_i)
{
    close(pipe_g_i[0]);

    int automobili[NUM_AUTO];

    for (int i = 0; i < NUM_AUTO; i++)
    {
        automobili[i] = EstraiDirezione(i);
        printf("Automobile numero %d, strada %d\n", i, automobili[i]);

        int wRet = write(pipe_g_i[1], automobili, sizeof(automobili));

        if (wRet == -1)
        {
            perror("Garage: Errore in write\n");
            exit(EXIT_FAILURE);
        }
        

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
    printf("Tutte le auto transitate\n");
    
    sleep(1);

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
    if (pipe_in == -1)
    {
        perror("Automobile: errore in apertura della pipe per l'incrocio\n");
        exit(EXIT_FAILURE);
    }
    int msg;

    int rRet = read(pipe_in, &msg, sizeof(msg));

    if (rRet == -1)
    {
        perror("Automobile: errore in lettura\n");
        exit(EXIT_FAILURE);
    }

    if (msg == PROCEDI)
    {
        int msg = AUTO_TRANSITATA;
        printf("Automobile %d: procedo nell'attraversamento\n", numAuto);
        write(pipe_out, &msg, sizeof(msg));
    }else{
        printf("Automobile: errore in lettura\n");
        exit(EXIT_FAILURE);
    }
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
