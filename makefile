#-#-# DECLARACAO DE VARIAVEIS #-#-#

COMPILER=gcc

PARAMS=-Wall

EXECS=teste main

OBJECTS=main.o archiver.o avl.o fila.o

PROGRAM_NAME=main

#-#-# REGRAS DE COMPILACAO #-#-#

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJECTS)
	$(COMPILER) $(PARAMS) $(OBJECTS) -o $(PROGRAM_NAME)

main.o: main.c
	$(COMPILER) $(PARAMS) -c main.c

fila.o: fila.h fila.c
	$(COMPILER) $(PARAMS) -c fila.c

avl.o: avl.h avl.c
	$(COMPILER) $(PARAMS) -c avl.c

archiver.o: archiver.h archiver.c
	$(COMPILER) $(PARAMS) -c archiver.c

teste:
	$(COMPILER) $(PARAMS) teste.c -o teste

clean:
	rm -f $(OBJECTS) $(EXECS)
