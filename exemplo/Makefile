#lct2fs
# Makefile para programas exemplo
#

CC=gcc
LIB_DIR=../lib
INC_DIR=../include
BIN_DIR=../bin
SRC_DIR=../src

all: t2shell

t2shell: t2shell.c $(LIB_DIR)/libt2fs.a
	$(CC) -o t2shell t2shell.c -L$(LIB_DIR) -I$(INC_DIR) -lt2fs -Wall

clean:
	rm -rf t2shell *.o *~
