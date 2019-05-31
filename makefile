GCC = gcc -Wall
MPICC = mpicc -Wall
GCCFLAGS = -c
MPICCFLAGS = -fopenmp
WORDSCOUNT = wordscount
FILE_UTILS = file_utils
CHUNKS = chunks
WORDSCOUNT_CORE = wordscount_core

#Being the first, it is called by only 'make'
all: $(WORDSCOUNT)
	$(RM) *.o
	@echo Build terminated successfully

$(WORDSCOUNT): $(WORDSCOUNT).c $(FILE_UTILS) $(CHUNKS) $(WORDSCOUNT_CORE)
	@echo Creating $(WORDSCOUNT) executable
	$(MPICC) $(MPICCFLAGS) -o $(WORDSCOUNT) $(WORDSCOUNT).c $(WORDSCOUNT_CORE).o $(CHUNKS).o $(FILE_UTILS).o
	@echo Builded $(WORDSCOUNT)

$(WORDSCOUNT_CORE): $(WORDSCOUNT_CORE).c $(WORDSCOUNT_CORE).h
	@echo Compiling $(WORDSCOUNT_CORE)
	$(GCC) $(GCCFLAGS) $(WORDSCOUNT_CORE).c
	@echo Builded $(WORDSCOUNT_CORE)

$(CHUNKS): $(CHUNKS).c $(CHUNKS).h
	@echo Compiling $(CHUNKS)
	$(GCC) $(GCCFLAGS) $(CHUNKS).c
	@echo Builded $(CHUNKS)

$(FILE_UTILS): $(FILE_UTILS).c $(FILE_UTILS).h
	@echo Compiling $(FILE_UTILS)
	$(GCC) $(GCCFLAGS) $(FILE_UTILS).c
	@echo Builded $(FILE_UTILS)

clean:
	@echo Cleaning everything
	$(RM) $(WORDSCOUNT)
	$(RM) *.o
	@echo Cleaning terminated