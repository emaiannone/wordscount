# Wordscount
Wordscount allows the counting of words from a large number of files using multiple processors of multiple nodes in a cluster. It is implemented in C using OpenMPI.

## Hypothesis
Some hypothesis have been made:
1. Each text file has **a single word per line**;
2. Instead of having the master reading a file containing the names of files to be processed, the **master process scans a directory that contains all input files**. This default directory is *files*, but it can be specified as a CLI parameter.
3. **Each process sees all input files**. As for cloud environment, each node should see the same files in the very same directory structure. So, the master node will be in charge for sending each slave node these files properly.
4. **The sequential solution** (where -np parameter is 1) **avoids MPI calls totally**, with the exception of *MPI_Init()*, *MPI_Finalize()* and *MPI_Wtime()*.

## General idea
Clearly, an algorithm that solves this problem will require a lot of disk accesses, independently if the provided solution is sequential or parallel.  
- **The sequential solution** is trivial: a single process reads all files;  
- **The parallel solution** acts like this:
  - Master process reads all files in order to get their **line number** and the **total line number**;
  - Master creates the **chunks** of works. A chunk is a structure that contains:
    - The file name;
    - The starting line;
    - The ending line.
  - So, each process might work on some lines of a single file, on a whole file or on different lines of consecutive files.  
  These chunks are created as a **linked list** because master do not know yet the total number of chunk that will be created.  
  **The rule is**: given *p* as the number of processor, all processes will receive at least `totalLineNumber / p` lines by combining a variable number of chunks; the first `totalLineNumber % p` processes will receive an extra line;
  - The number of chunks sent to each process is variable, so the master sends them the **number of chunks** that each process should expect with an *MPI_Scatter()*. After this, the chunks are put into a **contigous buffer** and sent with an *MPI_Scatterv()*;
  - Each process execute the **wordscount function** and creating a histogram as linked list of **words**, a structure that contains:
    - Word string;
    - Number of occurances of the word.
  - The size of each histogram is variable, so the slaves send the master the **size of each histogram** that it should expect with an *MPI_Gather()*. After this, the histograms are put into **contigous buffers** and sent with an *MPI_Gatherv()*; 
  - Master compacts all the histograms arrays into **a single big linked list**, printed on *stdout*.

## Program structure
The project is composed by 4 C source file:
1. *file_utils.c* deals with all the aspects related to **directory and file access**, for example to get file name and line numbers;
2. *chunks.c* deals with all the aspects related to **chunk creation as list and convertion to array**;
3. *wordscount_core.c* deals with all the aspects related to main **wordscount algorithm**, the **histogram conversion and compaction**;
4. *wordscount.c* has the **main()** function and deals with all the MPI aspects, like **initiallization**, **termination**, creation and commit of **custom datatypes**, **scattering**, **gathering**, and **time profiling**.

With the exception of *wordscount.c*, each file has a related header file.  
All these files are builded together with **make**.  
The dependencies are:
- C standard library;
- *string.h*;
- *dirent.h*
- *mpi.h*.

*file_utils.c* does not depend on any other source file; *chunks.c* depends on *file_utils.c*; *wordscount.c* depends on *wordscount_core.c* that depend on *chunks.c*.  

### Inputs and outputs
The program takes **two optional CLI parameters**:
1. The first one is the **file input directory**, the **relative path to the directory containing all the file to be processed**. If the parameter is not specified, a default *files* directory is searched in the current working directory. If the parameter is specified but there is no directory with that name, an error is returned.
2. The second one is **time log file name**, that is the **name of the file that will store all time profiling data**. If the parameter is not specified, a default *time_log* file is used. The file is created if it did not exist already, but if it already existed it will be opened in append.
The file input directory contains a several text files, basically named fileX.txt, where X is a number, tipically.  
The final output of the program is the compacted histogram, printed in the *stdout*.

### Time logging
All time logging data are first printed on *stdout* and then written on the time log file.  
The program has different phases, each of them has a time profile:
- **Init** is the time that master process took for the *initializaion phase*. In this phase the master:
  - Get all files names;
  - Get all files lines;
  - Prepare the chunks. 
- **Scattering** is the time that master process took for scattering the chunks to slave processes and the time that slaves took for receiving their chunks. This time is present as a **arithmethic mean of all scatting times**;
- **Wordscount** is the time that a process took to execute wordscount algorithm on its chunks. This time is present as a **arithmethic mean of all wordscount times**;
- **Gathering** is the time that a slave took for sending its histogram back to master and the time that master took for gathering all the histograms. This time is present as a **arithmethic mean of all gathering times**;
- **Compaction** is the time that master took for compacting all the histograms into a single one.
- **Global** is the time that a process took for its entire execution. This time is present as a **arithmethic mean of all global times**.

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
Launching the program requires two steps:
1. **Building**, easily done by launching the command `make` (the default target builds everything into a single executable named **wordscount**);
2. **Running**, done by launching the command `mpirun -np <#processes> wordscount` where <#processes> is the number of required processes. Optionally, it is possible to specify the file input directory and time log file, for example (for two processes): `mpirun -np 2 wordscount my_files_dir my_time_logfile`.

### Benchmarks Scripts
To easiliy launch benchmarks, three different shell script (.sh) files have been provided:
- **run_strong_benchmark.sh** that launches wordscount **multiple times using a different number of processes for each run but keeping the same file input directory**. It accepts three CLI parameters:
  - Maximum number of process to be used for the benchmark;
  - File input directory path;
  - Time log file name.  

Example: `./run_strong_benchmark.sh 8 strong_files/1 strong_time_1_1` that launches wordscount 8 times, with 1, 2, ..., 8 processes, using each time *strong_files/1* directory to get input files and using *strong_time_1_1* as time log file.
- **run_weak_benchmark.sh** that launches wordscount **multiple times using a different number of processes for each run but keeping the same work load for each process**. It accepts three CLI parameters:
  - Maximum number of process to be used for the benchmark;
  - File input directory parent path;
  - Time log file name.

Example: `./run_weak_benchmark.sh 8 weak_files/1 weak_time_1_1` that launches wordscount 8 times, with 1, 2, ..., 8 processes, using each time *weak_files/1* as parent directory where all input files are located and using *weak_time_1_1* as time log file. *weak_files/1* contains some directories named *pX*, where X is a number and the directory pI will be used in the I-th launch of wordscount, so in this example for the launch with 1 process, p1 is used, for the launch with 2 processes, p2 is used, etc.
- **run_benchmark.sh** that launches the **building with make** and if it is successful, it **launches strong and weak scalability scripts using ALL virtual processors on the machine multiplied by 5 (hardcoded in the scripts)**. It accepts two CLI parameters:
  - **Number of executions**, that is the number of times which the two kinds of benchmarks will be repeated. This is useful for the analysis of the benchmarks because a single benchmark might not be reliable.
  - **Machinefile**, when specified, the mpirun will use this machinefile as hostfile to run in "cloud mode", otherwise mpirun will run in "local mode".

This benchmark works on **3 different file input directories (hardcoded in the scripts)**.  
Each i-th exectution on the j-th file input directory creates two files: **strong_file_i_j** and **weak_file_i_j**, stored in two directories: *times/strong* for strong time logs and *times/weak* for weak time logs.

Example (local): `./run_benchmark.sh 4` that launches the benchmarks 4 times in local mode. If the local machine has 2 processors, the strong and weak scalability benchmarks will be launched 2 (processors) x 3 (instances) x 4 (rounds) = 24 times each, with a total of 48 executions of wordscount.

Example (cloud): `./run_benchmark.sh 4 machinefile` that launches the benchmarks 4 times in cloud mode. If the node has 4 processors, the strong and weak scalability benchmarks will be launched 2 (processors) x 5 (nodes) x 3 (instances) x 4 (rounds) = 120 times each, with a total of 240 executions of wordscount.

## General setup of Cloud Environment
In order to launch these benchmarks, we need a cluster of EC2 instances. These instances should have been 8 of **t2.xlarge, that have 4 vCPUs (Intel Xeon up to 3GHZ) and 16GiB of memory**. However, AWS Educate has a limit of 5 t2.xlarge instances, so **only 5 instances were used for this benchmark**.  
The choosen AMI is *ami-024a64a6685d05041*, an **Ubuntu Server 18.04 LTS machine with x86 architecture**.  
After creating the 5 instances:
- The send_to_master.tar.gz archive should be send from local to master node;
- Master node should copy this archive to pcpc user;
- Master node, on pcpc, should:
  - Unpack this archive;
  - Compile the whole project using `make`;
  - Create the right machinefile;
  - Repack the send_to_master directory into another tar.gz
  - Send the archive to all slave nodes;
- Each slave should only unpack the archive.
The setup is complete, now on pcpc@master the following command will run the benchmarks: `./run_benchmark.sh 4 machinefile`.

## Example of a benchmark
The local environment was a Ubuntu 19 run on VMWare, executed on Windows 10. The local machine has Intel Core i5 4 core, with Hyper-Threading enables, up to 3.4GHz and 8GB of memory.  
These are the steps followed to setup the cloud environment used to run the benchmarks:
1. Put **make-cluster-sh** in home directory (una tantum);
2. Put **send_to_master.tar.gz** in home directory (una tantum);
3. Create an **empty directory named *data*** in home directory (una tantum);
4. Create a **directory named *key*** in home directory that have the .pem key file (una tanum);
5. **Install jb** (una tantum): `sudo apt install jq`
6. Initialize the **AWS session**;
7. **Crate the EC2 cluster**: `./make_cluster.sh ami-024a64a6685d05041 ubuntu sg-09f21591cbc4c3e23 t2.xlarge devenv-key 5 pcpc pcpc` where:
    1. *ami-024a64a6685d05041*, the AMI code for the Ubuntu Server 18.04;
    2. *ubuntu*, the name of the default user of the master node;
    3. *sg-09f21591cbc4c3e23*, the ID of the selected security group (NOT the name);
    4. *t2.xlarge*, the istance type;
    5. *devenv-key*, the name of the selected key pair (the file in key directory must have this name, with .pem extension);
    6. *5*, the desiderd number of nodes;
    7. The first *pcpc*, the username of the user for the MPI execution;
    8. The second *pcpc*, the password of the user for the MPI execution.  
Note: This will install **OpenMPI 2.1.1**. Newer versions of MPI could be installed by unpacking the OpenMPI archive and the compiling it all, but this would require to much time to setup the whole cluster. The entire setup will require about 5 minutes;
8. **Send send_to_master.tar.gz to master node**: `scp -i key/devenv-key.pem send_to_master.tar.gz ubuntu@masterIP:/home/ubuntu`, where masterIP is to be replaced by the *master public IP*;
9. **Connect to master node**: `ssh -i key/devenv-key.pem ubuntu@masterIP`, where masterIP is to be replaced by the *master public IP*;
10. **Copy send_to_master.tar.gz to pcpc user**: `sudo cp send_to_master.tar.gz /home/pcpc`;
11. **Change user to pcpc**: `su - pcpc`. This will prompt the user to insert the password, that is *pcpc*;
12. **Unpack send_to_master.tar.gz**: `tar -zxvf send_to_master.tar.gz`;
13. **Go into send_to_master directory**: `cd send_to_master`
14. **Launch wordscount building**: `make`
15. **Launch VIM and create a file named *machinefile* with this content**:

        MASTER slots=4
        NODE_1 slots=4
        NODE_2 slots=4
        NODE_3 slots=4
        NODE_4 slots=4
    Note: These MASTER, NODE_1,... are values stored in */etc/hosts*, created when make_cluster.sh was launched; 
16. **Go back to home directory**: `cd ..`
17. **Remove the old send_to_master.tar.gz**: `rm -f send_to_master.tar.gz`
18. **Create a new send_to_master.tar.gz**: `tar -zcvf send_to_master.tar.gz send_to_master`
19. **Send the new send_to_master.tar.gz to each slave node**:
    1. `scp send_to_master.tar.gz pcpc@NODE_1:/home/pcpc`
    2. `scp send_to_master.tar.gz pcpc@NODE_2:/home/pcpc`
    3. `scp send_to_master.tar.gz pcpc@NODE_3:/home/pcpc`
    4. `scp send_to_master.tar.gz pcpc@NODE_4:/home/pcpc`
20. **Unpack send_to_master.tar.gz in each slave**:
    1. `ssh pcpc@NODE_1 tar -zxvf send_to_master.tar.gz`
    2. `ssh pcpc@NODE_2 tar -zxvf send_to_master.tar.gz`
    3. `ssh pcpc@NODE_3 tar -zxvf send_to_master.tar.gz`
    4. `ssh pcpc@NODE_4 tar -zxvf send_to_master.tar.gz`
21. **Go into send_to_master directory**: `cd send_to_master`
22. **Launch benchmarks**: `./run_benchmark.sh 4 machinefile`

### Graph evaluation
All the time profiling data are stored in *times* subdirectories. These data were manually gathered into a single text file, named *benchmark-times*, then a Python script, named *WordsCountGraphs.py*, reads these data and generates the graphs.

**Note I**: X-axis has number of processes, while y-axis has the running time (in ms) whose type depends on the kind of graph (for example, the global time graph has global times in y-axis).  
**Note II**: Each function is builded using **average times obtained from the 4 different executions** of the benchmarks in order to have more reliable times because there might be some time fluctuations.  
**Note III**: Each graph has **3 functions, each one representing a benchmark with different input files**. 
**Note IV**: The input files used for this benchmark are contained in *strong_files* and *weak_files* directories if send_to_master.tar.gz, but any other kind of text file can be used, under the condition of one word per line.

#### Input files details
*strong_files* directory has three subdirectories, each with 50% more files that the preceding:
  - *strong_files/1/* has 100 files for a total of 13.2 MB. Each process will work on `13.2 / np` MB
  - *strong_files/2/* 150 files for a total of 19.7 MB. Each process will work on `19.7 / np` MB
  - *strong_files/3/* 200 files for a total of 26.3 MB. Each process will work on `26.3 / np` MB
*weak_files* directory has three subdirectories, each with 50% more files that the preceding. Each directory has 20 pX subdirectories:
  - *weak_files/1/*, each pX has 10 * X files with a size of `665.1 * X` KB. Each process will always work on 665.1 KB distributed in 10 files.
  - *weak_files/2/*, each pX has 15 * X files with a size of `996.2 * X KB`. Each process will always work on 996.2 KB distributed in 15 files.
  - *weak_files/3/*, each pX has 20 * X files with a size of `1.3 * X MB`. Each process will always work on 1.3 MB distributed in 20 files.

#### Strong scalability
<p align="center"> 
<img src="https://github.com/emaiannone/wordscount/blob/master/figures/init_strong.png"><br>
  Being the init phase only done by master process, and being the input fixed independently from number of processes, the time function is <b>constant</b>. Besides, being each input directory greater by 50% that preceding one, the init time increases by 50%. So, as expected, the init phase does not scale at all.
</p>

![](https://github.com/emaiannone/wordscount/blob/master/figures/wordscount_strong.png)  
caption

![](https://github.com/emaiannone/wordscount/blob/master/figures/global_strong.png)  
caption

#### Weak scalability
![](https://github.com/emaiannone/wordscount/blob/master/figures/init_weak.png)  
caption

![](https://github.com/emaiannone/wordscount/blob/master/figures/wordscount_weak.png)  
caption

![](https://github.com/emaiannone/wordscount/blob/master/figures/global_weak.png)  
caption

#### Comparison
Strong | Weak
:---:|:---:
![](https://github.com/emaiannone/wordscount/blob/master/figures/init_strong.png)  |  ![](https://github.com/emaiannone/wordscount/blob/master/figures/init_weak.png)  

caption

(AVG Wordscount)
(AVG Global)

## Conclusions
There are some improvements that can be done:
- Performance: 
  - The init phase done by a single proces only is easy to implement but has bad performances. An idea might be to count the number of files and then equally divide the number of files among all processes to let them count the number of lines. However, despite each process will receive an equal number of files, each file might not be the same size as others, so some processes might work longer that other.
  - The conversion from a linked list to an array might seem trivial. Another option will be to send data directly from the linked list (uncontigous buffer), however the variable number of nodes to send is a bit tricky;
- Features:
  - Improve the wordscount algorithm in order to read from text files of any kind without the hypothesis of "one word per line". 
  - Create a more structure benchmarks_times file in order to be parsed easily
