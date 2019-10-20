#
# Makefile de EXEMPLO
#
# OBRIGATÓRIO ter uma regra "all" para geração da biblioteca e de uma
# regra "clean" para remover todos os objetos gerados.
#
# É NECESSARIO ADAPTAR ESSE ARQUIVO de makefile para suas necessidades.
#  1. Cuidado com a regra "clean" para não apagar o "support.o"
#
# OBSERVAR que as variáveis de ambiente consideram que o Makefile está no diretótio "cthread"
# 

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

CFLAGS= -Wall -Wextra -O2 -Wunreachable-code -Wuninitialized -Winit-self -std=gnu99
ARFLAGS= -rv


all: libt2fs.a

libt2fs.a: $(BIN_DIR)/t2fs.o 
	ar $(ARFLAGS) $(LIB_DIR)/$@ $^ $(LIB_DIR)/apidisk.o $(LIB_DIR)/bitmap2.o

$(BIN_DIR)/t2fs.o: $(SRC_DIR)/t2fs.c 
	$(CC) $(CFLAGS) -o $@ -c $^

.PHONY: clean

# We need to keep *.o files in LIB_DIR, so we delete only *.a
clean:
	rm -rf $(LIB_DIR)/*.a $(SRC_DIR)/*~ $(INC_DIR)/*~ $(BIN_DIR)/*~ *~


