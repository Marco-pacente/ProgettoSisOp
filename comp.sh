#usr/bin/bash

rm auto.txt 
rm incrocio.txt

mkdir tmp -p

gcc -o incrocio incrocio.c -Wall

RET=$?

printf "\n"
if test $RET -ne 0 
then
    echo "errore"
else
    ./incrocio
fi
