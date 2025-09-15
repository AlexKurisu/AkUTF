.POSIX:
.PHONY: all clean

CC=gcc
CFLAGS=-Og -ggdb -pipe
SANITIZE=-fsanitize=address,leak

ALL_WARN=-Wall -Wextra -Wshadow -Wpedantic -Wno-pointer-sign
ALL_DEF=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
ALL_CFLAGS=$(CFLAGS) $(ALL_DEF) -std=c99 -fno-strict-aliasing $(ALL_WARN)
LDLIBS=
OBJ=\
	akutf.o\
	cli.o
HDR=\
	akutf.h

all: cli

.c.o:
	$(CC) $(ALL_CFLAGS) $(SANITIZE) -c -o $@ $<

cli: $(OBJ)
	$(CC) $(LDFLAGS) $(SANITIZE) -o $@ $(OBJ) $(LDLIBS)

$(OBJ): $(HDR)

clean:
	rm -f cli $(OBJ)
