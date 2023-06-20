EXECS=teste main

all:
	gcc -Wall main.c -o main

teste:
	gcc -Wall teste.c -o teste

clean:
	rm -f $(EXECS)
