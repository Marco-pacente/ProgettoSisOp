#usr/bin/bash

gcc -o incrocio incrocio.c -Wall

RET=$?

printf ""
if test $RET -ne 0 
then
    echo "errore"
else
    ./incrocio
fi