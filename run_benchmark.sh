make
#TODO Propagate the executable wordscount and files to slave nodes
if [ $? -eq 0 ]
then
    STRONG_FILE="strong_times.txt"
    WEAK_FILE="weak_times.txt"
    FILE_DIR="files"
    N_CORES=$(grep -c ^processor /proc/cpuinfo)
    #Enable this on cloud n_cores=$n_cores*8

    rm $STRONG_FILE
    rm $WEAK_FILE

    #Strong scalability
    echo "---------------Strong scalability with $N_CORES process(es) on $FILE_DIR directory---------------"
    for ((i=1;i<=N_CORES;i++));
    do
        echo "---------------Lauching with $i process(es)---------------"
        mpirun -np $i wordscount $FILE_DIR $STRONG_FILE
    done

    #Weak scalability
    echo "---------------Weak scalability with $N_CORES process(es)---------------"
    for ((i=1;i<=N_CORES;i++));
    do
        echo "---------------Lauching with $i process(es)---------------"
        WORK_DIR=$FILE_DIR$i
        mpirun -np $i wordscount $WORK_DIR $WEAK_FILE
    done

else
    echo "Building errors: fix them before launching benchmarks"
fi