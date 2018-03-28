.phony all:
all: shell

shell: shell.c	
	gcc shell.c -lreadline -lhistory -ltermcap -o shell

.PHONY clean:
clean:
	-rm -rf *.o *.exe
