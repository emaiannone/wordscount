GCC = gcc
MPICC = mpicc
GCCFLAGS = -c
MPICCFLAGS = -fopenmp
TARGET = wordscount
FILE_UTILS = file_utils

#Being the first, it is called by only 'make'
all: $(TARGET)
	$(RM) *.o
	@echo Build terminated successfully

$(TARGET): $(TARGET).c $(FILE_UTILS)
	@echo Creating $(TARGET) executable
	$(MPICC) $(MPICCFLAGS) -o $(TARGET) $(TARGET).c $(FILE_UTILS).o

$(FILE_UTILS): $(FILE_UTILS).c $(FILE_UTILS).h
	@echo Compiling $(FILE_UTILS)
	$(GCC) $(GCCFLAGS) $(FILE_UTILS).c

clean:
	@echo Cleaning everything
	$(RM) $(TARGET)
	$(RM) *.o
	@echo Cleaning terminated