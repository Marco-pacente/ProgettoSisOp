#usr/bin/bash

rm auto.txt 
rm incrocio.txt

mkdir tmp -p

gcc -o incrocio incrocio.c -Wall

RET=$?

printf ""
if test $RET -ne 0 
then
    echo "errore"
else
    ./incrocio
fi

printf "differenze tra auto.txt e incrocio.txt:"
diff auto.txt incrocio.txt
printf ""