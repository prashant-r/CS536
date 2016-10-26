#!/usr/bin/env bash
totalservers=1
totalclients=1
count=0
joincount=0
a=''
while [ $count -lt $totalservers ]; do
let count++
./fileserver.o 54786 google.com configfile.dat &
done
echo "Server Started.. starting clients in 5 seconds"
secs=$((5))
while [ $secs -gt 0 ]; do
   echo -ne "$secs\033[0K\r"
   sleep 1
   : $((secs--))
done
b=0
echo "Joining clients"
while [ $b -lt $totalclients ]; do
let b++
./fileclient.o localhost 54786 google.com kline-jarrett.au configfile.dat &
done