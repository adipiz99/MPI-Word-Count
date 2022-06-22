all : main.o fileManagement.o wordManagement.o
	mpicc main.o fileManagement.o wordManagement.o -o wordCount.out

main.o :
	mpicc -c main.c 
	
fileManagement.o :
	mpicc -c fileManagement.c

wordManagement.o:
	mpicc -c wordManagement.c
	
clean: 
	rm -f wordCount.out main.o fileManagement.o wordManagement.o 