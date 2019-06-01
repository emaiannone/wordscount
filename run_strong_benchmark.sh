if [ -z $1 ]
then
    echo "Supply time log file name"
else
    STRONG_FILE=$1
    FILE_DIR="files"
    N_CORES=$(grep -c ^processor /proc/cpuinfo)
    #Enable this on cloud n_cores=$n_cores*8

    rm -f $STRONG_FILE
    #Strong scalability
    echo "---------------Strong scalability with $N_CORES process(es) on $FILE_DIR directory---------------"
    for ((i=1;i<=N_CORES;i++));
    do
        echo "---------------Lauching with $i process(es)---------------"
        mpirun -np $i wordscount $FILE_DIR $STRONG_FILE
    done
fi