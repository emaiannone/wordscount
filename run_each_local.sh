#!/bin/bash
#For each vCPU
n_cores=$(grep -c ^processor /proc/cpuinfo)
for ((i=1;i<=n_cores;i++));
do
    echo "---------------Lauching with $i process(es)---------------"
    mpirun -np $i wordscount $1 $2
done