mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
Glib-dir := $(shell pkg-config --cflags --libs glib-2.0)

wordCount: wordManagement.o fileManagement.o main.o
	mpicc $(Glib-dir) $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o -o $(mkfile_dir)src/wordCount.out

main.o:
	mpicc -c $(Glib-dir) $(mkfile_dir)src/main.c 
	
fileManagement.o :
	mpicc -c $(Glib-dir) $(mkfile_dir)src/fileManagement.c

wordManagement.o:
	mpicc -c $(Glib-dir) $(mkfile_dir)src/wordManagement.c
	
clean: 
	rm -f $(mkfile_dir)src/wordCount.out $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o 