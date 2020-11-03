build: editor
tema3: editor.c
	mpicc -o editor editor.c -Wall
clean:
	rm -f editor.o