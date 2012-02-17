
declare -a RUNSTRINGS;

EXPECTED_ARGS=3

if [ $# -lt $EXPECTED_ARGS ]
then
  echo 
  echo "Usage: `basename $0` scenario_name number_of_processors \
run_start_number [stop_number]"
  echo " where run_start_number is the run number from which to start executing"
  echo 
  exit;
fi

RUNS=`./run -x $1 | grep Number | awk '{n = split($0, array," "); \
  print array[n]; }'`
LASTRUN=$RUNS
STOPRUN=$(($RUNS-1))
PROCESSORS=$2
RUNSTART=$3

if [ $# == $(($EXPECTED_ARGS+1)) ]; then 
  STOPRUN=$4
fi

RUNS=$(($STOPRUN-$RUNSTART)) 

PROCPERRUN=$(($RUNS/$PROCESSORS))

# RUNS is the id of the last run, the first starts with 0, so 
# the number of runs == $RUNS+1
if [ $(($RUNS+1)) -lt $PROCESSORS ]; then 
 echo "ERROR: you vave more processors than runs: $PROCESSORS vs $RUNS";
 exit;
fi;

index=$RUNSTART

echo "launched $RUNS runs in $PROCESSORS procs with $PROCPERRUN runs per proc \
starting from $RUNSTART to $STOPRUN"



for i in `seq 0 $RUNS`; do 
# Testing only:
# rand=$(($RANDOM%15 + 1))
# RUNSTRINGS[$i]="./run -u Cmdenv -c "$1" -r  $(($index+$i)) --sim-time-limit=${rand}s"
 RUNSTRINGS[$i]="./run -u Cmdenv -c "$1" -r  $(($index+$i)) "
done; 

launchedprocesses=0

for i in `seq 0 $(($PROCESSORS-1))`; do 
#  echo "Running "${RUNSTRINGS[$i]}
#  RUNSTRINGS[$i]=${RUNSTRINGS[$i]}
#  `(${RUNSTRINGS[$i]} >> /dev/null) &`;
  `(${RUNSTRINGS[$i]} &> logs/$1-$i.txt) &`;
  launchedprocesses=$(($launchedprocesses+1))
done;

STARTED=`date +%s`

sleep 1;
#
while [ 1 ];
 do 
 clear; 
 NUMPROC=`ps aux | grep -v grep | grep $1 | grep -c opp_run`;
 NOW=`date +%s`
 echo -en "\n\t$launchedprocesses/$(($RUNS+1)) \e[00;31m$1\e[00m processes "
 echo -e "launched, \e[00;31m$NUMPROC\e[00m running from $(( ($NOW-$STARTED)/3600 )) hours + $(( (($NOW-$STARTED)%3600)/60 )) minutes"
 echo -e "\t on host \e[00;32m$HOSTNAME\e[00m"
 if [ $NUMPROC -lt $PROCESSORS ]; then 
   if [ $launchedprocesses -lt $(($RUNS+1)) ]; then
     `(${RUNSTRINGS[$launchedprocesses]} &> logs/$1-$launchedprocesses.txt) &`;
     echo  "process $(($launchedprocesses+1)) started"
     launchedprocesses=$(($launchedprocesses+1))
   fi;
   if [ $NUMPROC -eq 0 ]; then
    if [ $launchedprocesses -ge $(($RUNS+1)) ]; then
      echo "run all the processes, exiting";
      exit 0;
    fi;
   fi;
 fi;
# ps aux | grep -v grep | grep $1 | grep opp_run
 sleep 1; 
done; 





#for i in `seq 0 $(($PROCESSORS-1))`; do 
# index=$(($RUNSTART+$i*$PROCPERRUN))
# RUNSTRINGS[$i]="opp_runall ./run -u Cmdenv -c "$1" -r "$index-$(($index+$PROCPERRUN-1)) 
#done; 
#
#REMINDER=$(($RUNS-$(($PROCPERRUN*$PROCESSORS))))
#
#if [ $REMINDER != 0 ]; then
# for i in `seq 0 $(($REMINDER-1))`; do 
#  index=$(($RUNS-$REMINDER+$i+$RUNSTART))
#  RUNSTRINGS[$i]=${RUNSTRINGS[$i]}","$index
# done; 
#fi;
#
#for i in `seq 0 $(($PROCESSORS-1))`; do 
#  echo "Running "${RUNSTRINGS[$i]}
#  RUNSTRINGS[$i]=${RUNSTRINGS[$i]}
##  `(${RUNSTRINGS[$i]} >> /dev/null) &`;
#  `(${RUNSTRINGS[$i]} &> logs/$1-$i.txt) &`;
#done;
