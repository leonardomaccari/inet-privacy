
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
STOPRUN=$RUNS
PROCESSORS=$2
RUNSTART=$3

if [ $# == $(($EXPECTED_ARGS+1)) ]; then 
  STOPRUN=$4
fi

RUNS=$(($STOPRUN-$RUNSTART+1)) 

PROCPERRUN=$(($RUNS/$PROCESSORS))

if [ $RUNS -lt $PROCESSORS ]; then 
 echo "ERROR: you vave more processors than runs: $PROCESSORS vs $RUNS";
 exit;
fi;

index=$RUNSTART

echo "running $RUNS runs in $PROCESSORS procs with $PROCPERRUN runs per proc \
starting from $RUNSTART to $STOPRUN"


for i in `seq 0 $(($PROCESSORS-1))`; do 
 index=$(($RUNSTART+$i*$PROCPERRUN))
 RUNSTRINGS[$i]="./run -u Cmdenv -c "$1" -r"$index-$(($index+$PROCPERRUN-1))
done; 

REMINDER=$(($RUNS-$(($PROCPERRUN*$PROCESSORS))))

if [ $REMINDER != 0 ]; then
 for i in `seq 0 $(($REMINDER-1))`; do 
  index=$(($RUNS-$REMINDER+$i+$RUNSTART))
  RUNSTRINGS[$i]=${RUNSTRINGS[$i]}","$index
 done; 
fi;

for i in `seq 0 $(($PROCESSORS-1))`; do 
  echo "Running "${RUNSTRINGS[$i]}
  RUNSTRINGS[$i]=${RUNSTRINGS[$i]}
  `(${RUNSTRINGS[$i]} >> /dev/null) &`;
done;
