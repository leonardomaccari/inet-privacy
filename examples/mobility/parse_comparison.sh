
rm results/$1-$2-contacttime.ps results/$1-$2-interctime.ps

awk '/CONTACTTIME/, $NF ~ /INTERCTIME/' $1 > $1-contacttime.txt
awk '/INTERCTIME/, $NF ~ /ENDSTATISTICS/' $1 > $1-interctime.txt

awk '/CONTACTTIME/, $NF ~ /INTERCTIME/' $2 > $2-contacttime.txt
awk '/INTERCTIME/, $NF ~ /ENDSTATISTICS/' $2 > $2-interctime.txt

echo " set terminal postscript; set output \"results/$1-$2-contacttime.ps\"; set \
log y; plot \"$1-contacttime.txt\" using 1:3, \
\"$2-contacttime.txt\" using 1:3"  | gnuplot -persist

echo " set terminal postscript; set output \"results/$1-$2-interctime.ps\"; set \
log y; plot \"$1-interctime.txt\" using 1:3, \"$2-interctime.txt\" using 1:3"  | gnuplot -persist

rm $1-contacttime.txt $1-interctime.txt
rm $2-contacttime.txt $2-interctime.txt

