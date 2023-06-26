#-#-# DECLARACAO DE VARIAVEIS #-#-#

COMPILER=gcc

PARAMS=-Wall

DIR=src

OBJECTS=$(DIR)/main.o $(DIR)/archiver.o $(DIR)/avl.o $(DIR)/fila.o

PROGRAM_NAME=main

#-#-# REGRAS DE COMPILACAO #-#-#

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJECTS)
	$(COMPILER) $(PARAMS) $(OBJECTS) -o $(PROGRAM_NAME)

main.o: $(DIR)/main.c
	$(COMPILER) $(PARAMS) -c $(DIR)/main.c

fila.o: $(DIR)/fila.h $(DIR)/fila.c
	$(COMPILER) $(PARAMS) -c $(DIR)/fila.c

avl.o: $(DIR)/avl.h $(DIR)/avl.c
	$(COMPILER) $(PARAMS) -c $(DIR)/avl.c

archiver.o: $(DIR)/archiver.h $(DIR)/archiver.c
	$(COMPILER) $(PARAMS) -c $(DIR)/archiver.c

clean:
	rm -f $(OBJECTS) $(PROGRAM_NAME)
