mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

wordCount: linkedList.o linkedFileList.o fileManagement.o wordManagement.o main.o
	mpicc $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o $(mkfile_dir)src/linkedList.o $(mkfile_dir)src/linkedFileList.o -o $(mkfile_dir)src/wordCount.out

main.o:
	mpicc -c $(mkfile_dir)src/main.c -o $(mkfile_dir)src/main.o

linkedList.o:
	mpicc -c $(mkfile_dir)src/linkedList.c -o $(mkfile_dir)src/linkedList.o

linkedFileList.o:
	mpicc -c $(mkfile_dir)src/linkedFileList.c -o $(mkfile_dir)src/linkedFileList.o
	
fileManagement.o:
	mpicc -c $(mkfile_dir)src/fileManagement.c -o $(mkfile_dir)src/fileManagement.o

wordManagement.o:
	mpicc -c $(mkfile_dir)src/wordManagement.c -o $(mkfile_dir)src/wordManagement.o
	
clean: 
	rm -f $(mkfile_dir)src/wordCount.out $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o $(mkfile_dir)src/linkedList.o $(mkfile_dir)src/linkedFileList.o