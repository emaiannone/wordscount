if [ -z $1 ]
then
    echo "Supply time log file name"
else
    if [ -z $2 ]
    then
        echo "Supply maximum number of processes"
    else
        FILE_DIR="files"
        WEAK_FILE=$1
        rm -f $WEAK_FILE
        N_CORES=$2
        echo "---------------Weak scalability with $N_CORES process(es) on $FILE_DIR directory---------------"
        for ((i=1;i<=N_CORES;i++));
        do
            echo "---------------Lauching with $i process(es)---------------"
            WORK_DIR=$FILE_DIR$i
            mpirun -np $i wordscount $WORK_DIR $WEAK_FILE
        done
    fi
fi