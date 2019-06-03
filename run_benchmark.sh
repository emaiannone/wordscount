if [ -z $1 ]
then
    echo "Supply number of executions"
else
    make
    #TODO Propagate the executable wordscount and files and machinefile to slave nodes
    if [ $? -eq 0 ]
    then
        N_CORES=$(grep -c ^processor /proc/cpuinfo)
        # TODO Enable this line on cloud
        # n_cores=$n_cores*8
        
        # Creation of directory structure
        STRONG_DIR="times/strong"
        WEAK_DIR="times/weak"
        rm -rf "times"
        mkdir -p $STRONG_DIR
        mkdir -p $WEAK_DIR

        STRONG_FILE="strong_times"
        WEAK_FILE="weak_times"
        N_EXECUTIONS=$1

        # Strong scalability
        for ((i=1;i<=N_EXECUTIONS;i++));
        do
            echo "---------------Execution #$i---------------"
            STRONG_FILE_I=$STRONG_FILE"_"$i
            ./run_strong_benchmark.sh $STRONG_FILE_I $N_CORES
            mv $STRONG_FILE_I $STRONG_DIR
        done
        # Weak scalability
        for ((i=1;i<=N_EXECUTIONS;i++));
        do
            echo "---------------Execution #$i---------------"
            WEAK_FILE_I=$WEAK_FILE$i
            ./run_weak_benchmark.sh $WEAK_FILE_I $N_CORES
            mv $WEAK_FILE_I $WEAK_DIR
        done
    else
        echo "Building errors: fix them before launching benchmarks"
    fi    
fi