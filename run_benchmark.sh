if [ -z $1 ]
then
    echo "Supply number of executions for greater precision"
else
    make
    if [ $? -eq 0 ]
    then
        N_EXECUTIONS=$1
        # This is fixed at three instances, but it can be freely set
        N_INSTANCES=3

        N_CORES=$(grep -c ^processor /proc/cpuinfo)
        # This is needed to count the 5 nodes of the cluster
        #N_CORES=$(($N_CORES * 5))

        # Base directory for input files
        STRONG_INPUT_DIR="strong_files"
        WEAK_INPUT_DIR="weak_files"
        
        # Creation of time log directory structure
        rm -rf "times"
        STRONG_LOG_DIR="times/strong"
        WEAK_LOG_DIR="times/weak"
        mkdir -p $STRONG_LOG_DIR
        mkdir -p $WEAK_LOG_DIR
        STRONG_LOG_FILE="strong_times"
        WEAK_LOG_FILE="weak_times"

        for ((i=1;i<=N_INSTANCES;i++));
        do
            STRONG_INPUT_DIR_I=$STRONG_INPUT_DIR"/"$i
            # Strong scalability
            for ((j=1;j<=N_EXECUTIONS;j++));
            do
                echo "---------------Execution #$j---------------"
                STRONG_LOG_FILE_IJ=$STRONG_LOG_FILE"_"$i"_"$j
                ./run_strong_benchmark.sh $N_CORES $STRONG_INPUT_DIR_I $STRONG_LOG_FILE_IJ
                mv $STRONG_LOG_FILE_IJ $STRONG_LOG_DIR
            done
        done

        for ((i=1;i<=N_INSTANCES;i++));
        do
            WEAK_INPUT_DIR_I=$WEAK_INPUT_DIR"/"$i
            # Weak scalability
            for ((j=1;j<=N_EXECUTIONS;j++));
            do
                echo "---------------Execution #$j---------------"
                WEAK_LOG_FILE_IJ=$WEAK_LOG_FILE"_"$i"_"$j
                ./run_weak_benchmark.sh $N_CORES $WEAK_INPUT_DIR_I $WEAK_LOG_FILE_IJ
                mv $WEAK_LOG_FILE_IJ $WEAK_LOG_DIR
            done
        done
    else
        echo "Building errors: fix them before launching benchmarks"
    fi    
fi