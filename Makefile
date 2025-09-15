.POSIX:
.PHONY: all clean test benchmark

CC=gcc
CFLAGS=-Og -ggdb -pipe
SANITIZE?=-fsanitize=address,leak

ALL_WARN=-Wall -Wextra -Wshadow -Wpedantic -Wno-pointer-sign -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
ALL_DEF=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
ALL_CFLAGS=$(CFLAGS) $(ALL_DEF) -std=c99 -fno-strict-aliasing -pedantic-errors $(ALL_WARN)
LDLIBS=
COMMON_OBJ=akutf.o
CLI_OBJ=$(COMMON_OBJ) cli.o
TEST_OBJ=$(COMMON_OBJ) test.o
HDR=\
	akutf.h

all: cli

test: tests
	./tests

benchmark: tests
	./tests --benchmark

.c.o:
	$(CC) $(ALL_CFLAGS) $(SANITIZE) -c -o $@ $<

cli: $(CLI_OBJ)
	$(CC) $(LDFLAGS) $(SANITIZE) -o $@ $(CLI_OBJ) $(LDLIBS)

tests: $(TEST_OBJ)
	$(CC) $(LDFLAGS) $(SANITIZE) -o $@ $(TEST_OBJ) $(LDLIBS)

$(COMMON_OBJ) cli.o test.o: $(HDR)

clean:
	rm -f cli tests $(COMMON_OBJ) cli.o test.o
