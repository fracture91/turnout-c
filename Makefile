# Makefile for TurnOut
#
# Last Updated 1/27/08 @ 1938 EDT

CC = gcc
DEBUG_FLAGS = -ggdb -Wall -DDEBUG_MODE
FLAGS = -fno-stack-protector
INCLUDES = -I/usr/include/mysql
LIB_DIRS = -L/usr/lib/mysql
LIBS = -lmysqlclient

all: turnout

turnout:
	$(CC) $(FLAGS) -w $(INCLUDES) $(LIB_DIRS) $(LIBS) -o TurnOut gradebook.c

debug:
	$(CC) $(FLAGS) $(DEBUG_FLAGS) $(INCLUDES) $(LIB_DIRS) $(LIBS) -o TurnOut gradebook.c

clean:
	/bin/rm -f *.core core.* TurnOut a.out
