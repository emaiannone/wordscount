# Wordscount
- Small intro

## Problem statement
- Describe the problem

## Hypothesis
- Each file has a single word per line
- Master does not read a file with filenames but get its from an input directory
- Each nodes sees files, organized in the same way
- Sequential solution avoid MPI calls excepts: Init, Finalize, Time

## Program structure
- Usage of make
- Dependencies
- number of source files and their role
- number of script files and their role
- instances directory
- How to run the program: make and then mpirun and input instance
- See time log file
- Archive

## Benchmarks on the cloud
- Describe the benchmark environment like the EC2 istance that should be used, their specs and and their number

### How to run a benchmark
- Create 8 EC2 instances that...
- Launch benchmark srcipt
- Wait
- See time log files in times directory: strong and weak

### Example of a benchmark
- Show how to setup the cloud environment  (provide the DESI script)
- Show how to launch benchmark script
- Show terminal of execution
- Show how to read time log files
- Evaluation graphs
- Exmplain which script has been used to make these plots
 
## Conclusions
- What can be changed to improve performances
