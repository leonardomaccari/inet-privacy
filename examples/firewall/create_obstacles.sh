
# Use this script to create a random set of squared obstacles 
# with a side included between obstaclesmaxside/2 and obstaclesmaxside
# and with a margin of space between each obstacle.

if [ $# != 4  ]; then
 echo "usage: ./create_obstacles.sh numobstacles obstaclesmaxside areasideX areasideY";
fi;
MARGIN=10
NUMOBSTACLES=$1
MAXSIDE=$2
AREASIDEX=$(($3-$MARGIN))
AREASIDEY=$(($4-$MARGIN))

echo "<obstacles>"
for i in `seq 0 $NUMOBSTACLES`; do
  side=`echo $RANDOM%$MAXSIDE | bc`;
  xmin=`echo $RANDOM%\($AREASIDEX-$side-2*$MARGIN\)+$MARGIN | bc`;
  ymin=`echo $RANDOM%\($AREASIDEY-$side-2*$MARGIN\)+$MARGIN | bc`;
  echo " <poly id=\"building#$i\" type=\"building\" color=\"#F00\" shape=\"$xmin,$ymin  $(($xmin+$side)),$ymin $(($xmin+$side)),$(($ymin+$side)) $xmin,$(($ymin+$side))\" />"
done;
echo "</obstacles>"

