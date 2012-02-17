#!/bin/sh

export LASTTIME="0"
RESFILE=$1
while [ `ps aux | grep -v grep | grep opp_run | grep -c $RESFILE` != "0" ]; 
 do 
 export LASTTIME=`ps ax -o comm,time,args | grep opp_run | grep $RESFILE | tail -n 1 | awk '{print $2}'`
 sleep 1; 
 clear; 
 NUMPROC=`ps aux | grep -v grep | grep -c opp_run`;
 echo -e "\n\t$NUMPROC \e[00;31m$1\e[00m processes running since $LASTTIME time"
 echo -e "\t on host \e[00;32m$HOSTNAME\e[00m"
done; 

play /usr/share/sounds/KDE-Im-Sms.ogg 2> /dev/null

echo -n "Global packets arrived ration: "
arrival=`grep 'Global\ packets\ arrived' results/$RESFILE-*sca | awk 'f += $7 {printf("%.13g\n",f/NR)}' | tail -n 1`
echo $arrival;

echo -n "ReceivedPk Hopcount: "
realhc=`grep  -C 3 "rcvdPkHCount:histogram" results/$RESFILE-*sca  | grep mean | awk '{print $3}' | grep -v 0 |  awk 'tot=tot+$1 {print tot/NR}'   | tail -n 1`

echo $realhc;
echo -n "RoutingTable Hopcount: "
rthc=`grep  -C 3 "hopCount:histogram" results/$RESFILE-*sca  | grep mean | awk 'tot=tot+$3 {print tot/NR}'  | tail -n 1`
echo $rthc;

echo "Run time of last simulation: $LASTTIME"

echo "|| || || || $arrival || $rthc || $realhc || $LASTTIME ||"
if [ $# == "2" ]; then
  echo "|| || || || $arrival || $rthc || $realhc || $LASTTIME ||" > $2
fi;
