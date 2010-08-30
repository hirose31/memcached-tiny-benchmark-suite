# -*- mode: makefile -*-
UNAME   = $(shell uname -s)
CC      = gcc
CFLAGS  = -O2 -g -Wall -I/usr/local/app/libmemcached/include
LDFLAGS = -L/usr/local/app/libmemcached/lib -lmemcached

ifeq ($(UNAME),Linux)
# ...
endif
ifeq ($(UNAME),FreeBSD)
# ...
endif

SOURCES  = load-mcd.c # $(shell echo *.c)
PROGRAMS = $(SOURCES:.c=)

all: $(PROGRAMS)

clean:
	$(RM) $(PROGRAMS) *~ core.*
