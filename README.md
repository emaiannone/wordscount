# Wordscount
Wordscount allows the reading of a large number of files and the couting of their words using multiple processors and nodes in a cluster. It is implemented in C using OpenMPI.

## Hypothesis
Some hypothesis have been made:
1. Each text file has a single word per line;
2. Instead of having the master reading a file containing the names of files to be processed, the master process scans a directory that contains all input files. This directory is *files* by default or it can be specified as a CLI parameter.
3. Each process is able to see all input files. As for cloud environment, each node should see the same files in the very same directory structure. So, the master node will be in charge for sending each slave node these files properly.
4. The sequential solution (where -np parameter is 1) will avoid MPI calls totally, with the exception of *MPI_Init()*, *MPI_Finalize()* and *MPI_Wtime()*.

## General idea
Clearly, an algorithm that solves this problem will require a lot of disk accesses, independently if the provided solution is sequential or parallel.  
- **The sequential solution** is trivial - a single process reads all files;  
- **The parallel solution** acts like this:
  - Master process reads all files in order to get their **line number** and the **total line number**;
  - Master creates the **chunks** of works. A chunk is a structure that contains:
    - The file name;
    - The starting line;
    - The ending line.
  - So, each process might work on some lines of a single file, on a whole file or on different lines of consecutive files.  
  These chunks are created as a **linked list** because master do not know yet the total number of chunk that will be created.  
  **The rule is**: given *p* as the number of processor, all processes will receive at least `totalLineNumber/p` lines by combining a variable number of chunks; the first `totalLineNumber%p` processes will receive an extra line.
  - The number of chunks sent to each process is not equal, so before sending them master process will send each process the number of chk that each process should except, this is done with a MPI_Scatter(). After this, the chunks are sent with an MPI_Scatterv().  
After the counting phase, each process send back the results in the form of histograms. Each histogram contains different number of words, so a MPI_Gather() to get the size of each histogram is needed first, then a MPI_Gatherv() to let the master gather all the different histograms.  
All the histograms are then compacted into a single one (as a linked list because it does not know yet the number of different words) that contains each words with total occurances.

## Program structure
The project is composed by 4 C source file: *file_utils.c*, *chunks.c*, *wordscount_core.c*, *wordcount.c*:
1. *file_utils.c* deals all aspects related to directory and file access, so getting their name and their lines;
2. *chunks.c* deals all aspect related to chunk creation as list and convertion to array;
3. *wordscount_core.c* deals all aspect related to main word couting algorithm that generate a histogram as linked list, histogram conversion to array and the histograms compaction into a single one;
4. *wordscount.c* has the main() function and deals all the MPI aspects, like initiallization, termination, creation and commit of custom datatypes, scattering, gathering, and time profiling.

### Build and dependencies
With the exception of wordscount.c, each file has a related header file. All these are builded together with **make**.  
The dependencies are on C standard library, *string.h*, *dirent.h* and, of cource, *mpi.h*. *file_utils.c* does not depend on any other file, and *chunks.c* depends on *file_utils.c*. *wordscount.c* depends on *wordscount_core.c* that depend on *chunks.c*.  

### Inputs and output
The program takes two optional CLI parameters:
1. The first one is the **file input directory**, that is the relative path to the directory containing all the file to be processed. If the parameter is not specified, a default *files* directory is searched in the current working directory. If it is specified but there is not directory with that name, an error is returned.
2. The second one is **time log file name**, that is the name of the file that will store all time profiling data. If the parameter is not specified, a default *time_log* file is used. The file is created if it does not exists but it exists it is opened in append. 

The file input directory contains a several text files, basically named fileX.txt, where X is a number tipically.  
The final output of the program is the compacted histogram, containing each different word and its related number of occurances. The output is printed in the *stdout*.

### Time logging
All time logging are first printed on stdout and then written on the time log file. These are different phases of the program, and for each of them the time is profiled:
- **Init** is the time that master process took for the *initializaion phase*. In this phase the master get all files names and lines from the file input directory and prepare the chunks, according to the number of involved processes p. If p is 1, a single chunk will be created for itself. 
- **Scattering** is the time that master took for scattering the data and slave to receive them. This time is present as a arithmethic mean of scatting times;
- **Wordscount** is the time that a process took to count words in its chunks and build its histogram. This time is present as a arithmethic mean of wordscount times;
- **Gathering** is the time that a slave took for sending their histograms and master to gather them all. This time is present as a arithmethic mean of gathering times;
- **Compaction** is the time that master took for compacting all the histograms into a single one.
- **Global** is the time that a process took for its entire execution. This time is present as a arithmethic mean of global times;

This is an example of a time log for a sequential execution:

    ---1 process(es) in ./files1/ directory---  
    Init, 0.003163  
    Average Wordscount, 0.006373  
    Compaction, 0.000004  
    Average Global, 0.090369  

While for parallel executions:

    ---6 process(es) in ./files6/ directory---
    Init, 0.026410
    Average Scattering, 0.000513
    Average Wordscount, 0.009540
    Average Gathering, 0.002535
    Compaction, 0.000014
    Average Global, 0.181598

### Building and running the program
To launch the program two steps are needed:
1. Building, easily done by launching the command `make` (default target is the builds everything into a single executable named **wordscount**)
2. Running, done by launching the command `mpirun -np <#processes> wordscount` where <#processes> is the number of required processes. Optionally, it is possible to specify the file input directory and time log file, for example: `mpirun -np 2 wordscount my_files_dir my_time_logfile`

### Benchmarks Scripts
To easiliy launch benchmark, three different shell script (.sh) files have been provided:
- **run_strong_benchmark.sh** that launches wordscount executable multiple times using a different number of processes for each run but keeping the same file input directory (fixed as *files* directory). It is also possibile to choose the name of the time log file that will be created. Example: `./run_strong_benchmark.sh strong_file 8` that launches wordscount 8 times, with 1, 2, ..., 8 processes and storing time logging in *strong_file*.
- **run_strong_benchmark.sh** that launches wordscount executable multiple times using a different number of processes for each run but *keeping the same work size per process*. Each run provide different file input directory, named *files1, files2, ..., filesn* where *filesi* has two copies of the files of *filesi-1*, for example files1 has 10 files, files2 has the two copies of the files of files1, so 20. It is also possibile to choose the name of the time log file that will be created. Example: `./run_weak_benchmark.sh weak_file 8` that launches wordscount 8 times, with 1, 2, ..., 8 processes and storing time logging in *weak_file*.
- **run_benchmark.sh** that launch the building using make and if it is successful, it launches strong and weak scalability script files using ALL virtual processors on the machine multiplied by 8. An extra CLI parameter must be provided to put the number of benchmark execution, useful for the analysis of the benchmark because a single benchmark might not be reliable.  
Each i exectution creates two file: **strong_file_i** and **weak_file_i**, stored in two directories: *times/strong* for strong time logs and *times/weak* for weak time logs. Example: `./run_benchmark.sh 4` that launches the benchmarks 4 times. If the machine has 2 processors, the strong and weak scalability benchmarks will be launched 2 x 4 = 8 times each, with a total of 2 x 4 x 2 = 16 launches of wordscount.

## Setup of Cloud Environment
In order to launch the benchmarks, we need a cluster of EC2 instances. These instances should have been 8 of t2.xlarge, that have 4 vCPUs (Intel up to 3GHZ) and 16GiB of memory. However, AWS Educate has a limit of 5 t2.xlarge instances, to only 5 instances were used for this benchmark.  
The AMI is *ami-024a64a6685d05041*, an Ubuntu Server 18.04 LTS machine with x86 architecture.  
After creating 5 instances, the send_to_master.tar.gz archive should be send to master node. Master should copy this archive to pcpc user and then unpack this archive, compile the whole project using make, and then create the machinefile. All this directory, with the source code, input files, executable file and machinefile, should be compressed again and sent to each slave nodes, on pcpc user. Slave should only unpack it.  
The setup is complete: on master node `./run_benchmark.sh 4` can be launched to launch 4 times strong and weak scalability benchmarks. All the result will be stored in *times* subdirectories. 

## Example of a benchmark
These are the steps followed to setup the cloud environment:
1. Creation of an **empty directory named *data*** in CWD if it did not exist before;
2. Creation of a **directory named *key*** in CWD that have the .pem key file, if it did not exist before;
3. Install of jb: `sudo apt install jq`
4. send_to_master.tar.gz whould be in the CWD
0. Init of AWS session
3. **Launch command**: `./make_cluster.sh ami-024a64a6685d05041 ubuntu sg-09f21591cbc4c3e23 t2.xlarge devenv-key 3 pcpc pcpc` where:
    3.1.  (installa OpenMPI 2.1.1 perché chiede poco tempo. La 4.0.0 richiede troppo tempo di compilazione) 
4. Waiting for instances creation It takes roughtly 5 minutes
5. **Send send_to_master.tar.gz to master node**: `scp -i key/devenv-key.pem send_to_master.tar.gz ubuntu@masterIP:/home/ubuntu` where masterIP should be replace by the *master public IP*;
6. **Connect to master node**: `ssh -i key/devenv-key.pem ubuntu@masterIP`, where masterIP should be replace by the **master public IP**
7. **Copy send_to_master.tar.gz to pcpc user**: `sudo cp send_to_master.tar.gz /home/pcpc`;
8. **Change user to pcpc**: `su - pcpc` and use *pcpc* as password;
9. **Unpack send_to_master.tar.gz**: `tar -zxvf send_to_master.tar.gz`;
10. **Go into send_to_master directory**: `cd send_to_master`
11. **Launch wordscount building**: `make`
12. **Launch VIM and create a file named *machinefile* with this content** (These MASTER, NODE_1,... value were stored in */etc/hosts* when make_cluster.sh was launched): 
    MASTER slots=4
    NODE_1 slots=4
    NODE_2 slots=4
    NODE_3 slots=4
    NODE_4 slots=4
13. **Ho back to home directory**: `cd ..`
14. **Remove the old send_to_master.tar.gz**: `rm -f send_to_master.tar.gz`
15. **Create a new send_to_master.tar.gz**: `tar -zcvf send_to_master.tar.gz send_to_master`
16. **Send the new send_to_master.tar.gz to each slave node**: 
    16.1 `scp send_to_master.tar.gz pcpc@NODE_1:/home/pcpc`
    16.2 `scp send_to_master.tar.gz pcpc@NODE_2:/home/pcpc`
    16.3 `scp send_to_master.tar.gz pcpc@NODE_3:/home/pcpc`
    16.4 `scp send_to_master.tar.gz pcpc@NODE_4:/home/pcpc`
17. **Unpack send_to_master.tar.gz in each slave**:
    17.1 `ssh pcpc@NODE_1 tar -zxvf send_to_master.tar.gz`
    17.2 `ssh pcpc@NODE_2 tar -zxvf send_to_master.tar.gz`
    17.3 `ssh pcpc@NODE_3 tar -zxvf send_to_master.tar.gz`
    17.4 `ssh pcpc@NODE_4 tar -zxvf send_to_master.tar.gz`
17. **Go into send_to_master directory**: `cd send_to_master`
18. **Go into send_to_master directory**: `./run_benchmark.sh 4`

The setup is complete: on master node the following command was used: `./run_benchmark.sh 4` in order to launch 4 times strong and weak scalability benchmarks. All the result were be stored in *times* subdirectories, so all data were gathered and written in *graphs.py* Python script file, in order to generate the benchmark graphs:

### Graph Evaluation
- Evaluation graphs
 
## Conclusions
- What can be changed to improve performances
