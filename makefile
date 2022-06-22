mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
Glib-dir := $(shell pkg-config --cflags --libs glib-2.0)

wordCount: fileManagement.o wordManagement.o main.o
	mpicc $(Glib-dir) $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o -l glib-2.0 -o $(mkfile_dir)src/wordCount.out

main.o:
	mpicc -c $(Glib-dir) $(mkfile_dir)src/main.c -o $(mkfile_dir)src/main.o
	
fileManagement.o:
	mpicc -c $(Glib-dir) $(mkfile_dir)src/fileManagement.c -o $(mkfile_dir)src/fileManagement.o

wordManagement.o:
	mpicc -c $(Glib-dir) $(mkfile_dir)src/wordManagement.c -o $(mkfile_dir)src/wordManagement.o
	
clean: 
	rm -f $(mkfile_dir)src/wordCount.out $(mkfile_dir)src/main.o $(mkfile_dir)src/fileManagement.o $(mkfile_dir)src/wordManagement.o 