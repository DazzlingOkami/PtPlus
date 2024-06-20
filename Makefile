CC=gcc
CFLAGS= -Wall -lm

ifeq ($(OS),Windows_NT)
# Windows
else
# Linux or other
endif

CFLAGSR=$(CFLAGS) -Ofast
CFLAGSD=$(CFLAGS) -g -DDEBUG=1

CFILES  = $(wildcard protothreads/*.c) pt_plus.c test.c
RFILES  = $(CFILES) $(wildcard protothreads/*.h) pt_plus.h list.h

test: $(RFILES) 
	$(CC) -o test $(CFLAGSR) $(CFILES)
	./test
