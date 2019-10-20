#
# Makefile para programas exemplo
# 

CC=gcc
LIB_DIR=../lib

all: t2shell

t2shell: t2shell.c $(LIB_DIR)/libt2fs.a
	$(CC) -o t2shell t2shell.c -L$(LIB_DIR) -lct2fs -Wall

clean:
	rm -rf t2shell *.o *~




