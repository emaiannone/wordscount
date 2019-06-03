if [ -z $1 ]
then
    echo "Supply time log file name"
else
    if [ -z $2 ]
    then
        echo "Supply maximum number of processes"
    else
        FILE_DIR="files"
        STRONG_FILE=$1
        rm -f $STRONG_FILE
        N_CORES=$2
        echo "---------------Strong scalability with $N_CORES process(es) on $FILE_DIR directory---------------"
        for ((i=1;i<=N_CORES;i++));
        do
            echo "---------------Lauching with $i process(es)---------------"
            mpirun -np $i wordscount $FILE_DIR $STRONG_FILE
        done
    fi
fi