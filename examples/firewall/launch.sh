#!/bin/sh
configs="MeshMiddleFastTestIII MeshMiddleSlowTestIV"
for i in $configs; do
 ./parallel_runs.sh $i 8 0 ; sleep 1;./parse_res.sh  $i $i-out.log
done;
