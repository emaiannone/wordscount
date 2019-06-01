make
#TODO Propagate the executable wordscount and files and machinefile to slave nodes
if [ $? -eq 0 ]
then
    STRONG_FILE="strong_times"
    WEAK_FILE="weak_times"
    STRONG_DIR="times/strong"
    WEAK_DIR="times/weak"
    if [ -z $1 ]
    then
        echo "Supply number of executions"
    else
        rm -rf "times"
        mkdir -p $STRONG_DIR
        mkdir -p $WEAK_DIR

        N_EXECUTIONS=$1
        for ((i=1;i<=N_EXECUTIONS;i++));
        do
            echo "---------------Execution #$i---------------"
            STRONG_FILE_I=$STRONG_FILE"_"$i
            ./run_strong_benchmark.sh $STRONG_FILE_I
            mv $STRONG_FILE_I $STRONG_DIR
        done

        for ((i=1;i<=N_EXECUTIONS;i++));
        do
            echo "---------------Execution #$i---------------"
            WEAK_FILE_I=$WEAK_FILE$i
            ./run_weak_benchmark.sh $WEAK_FILE_I
            mv $WEAK_FILE_I $WEAK_DIR
        done
    fi
else
    echo "Building errors: fix them before launching benchmarks"
fi