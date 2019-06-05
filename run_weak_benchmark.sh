if [ -z $1 ]
then
    echo "Supply maximum number of processes"
else
    if [ -z $2 ]
    then
        echo "Supply files directory"
    else
        if [ -z $3 ]
        then
            echo "Supply time log file name"
        else
            N_CORES=$1
            FILE_DIR=$2
            TIME_LOG_FILE=$3
            MACHINEFILE=$4
            rm -f $TIME_LOG_FILE
            echo ">>>>>>>>>>Weak scalability $N_CORES process(es) on $FILE_DIR<<<<<<<<<<"
            for ((i=1;i<=N_CORES;i++));
            do
                echo "-----Lauching $i process(es)-----"
                WORK_DIR=$FILE_DIR"/p"$i
                if [ -z $MACHINEFILE ]
                then
                    mpirun -np $i wordscount $WORK_DIR $TIME_LOG_FILE
                else
                    mpirun -np $i --hostfile $MACHINEFILE wordscount $WORK_DIR $TIME_LOG_FILE
                fi
            done
        fi
    fi
fi