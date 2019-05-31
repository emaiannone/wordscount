for i in 1 2 3 4 5 6 7 8
do
echo "---------------Lauching with $i process(es)---------------"
mpirun -np $i wordscount
done
