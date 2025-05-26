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
    
    int autoFd = open("auto.txt", O_CREAT|O_WRONLY, S_IWUSR|S_IRUSR);
    if (autoFd == -1)
    {
        perror("Automobile: errore in apertura del file auto.txt\n");
        exit(EXIT_FAILURE);
    }
    


    if (msg == PROCEDI)
    {
        int msg = AUTO_TRANSITATA;
        printf("Automobile %d: procedo nell'attraversamento\n", numAuto);
        char line[2];
        
        line[0] = numAuto+'0';
        line[1] = '\0';
        
        if (write(autoFd, line, strlen(line)) == -1)
        {
            perror("Automobile: errore in scrittura sul file\n");
            exit(EXIT_FAILURE);
        }
        if (write(pipe_out, &msg, sizeof(msg)) == -1)
        {
            perror("Automobile: errore in scrittura su pipe\n");
            exit(EXIT_FAILURE);
        }
        

    }else{
        printf("Automobile: errore in lettura\n");
        exit(EXIT_FAILURE);
    }
    return;
}