if [ -z $1 ]
then
    echo "Supply time log file name"
else
    WEAK_FILE=$1
    FILE_DIR="files"
    N_CORES=$(grep -c ^processor /proc/cpuinfo)
    #Enable this on cloud n_cores=$n_cores*8

    rm -f $WEAK_FILE
    #Weak scalability
    echo "---------------Weak scalability with $N_CORES process(es)---------------"
    for ((i=1;i<=N_CORES;i++));
    do
        echo "---------------Lauching with $i process(es)---------------"
        WORK_DIR=$FILE_DIR$i
        mpirun -np $i wordscount $WORK_DIR $WEAK_FILE
    done
fi