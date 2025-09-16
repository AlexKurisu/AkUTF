.POSIX:
.PHONY: all clean test benchmark
.DEFAULT: all

CC=gcc
CFLAGS=-Og -ggdb -pipe
SANITIZE?=-fsanitize=address,leak

ALL_WARN=-Wall -Wextra -Wshadow -Wpedantic -Wno-pointer-sign -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
ALL_DEF=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
ALL_CFLAGS=$(CFLAGS) $(ALL_DEF) -std=c99 -fno-strict-aliasing -pedantic-errors $(ALL_WARN)
LDLIBS=
COMMON_OBJ=src/akutf.o src/utf8_string.o src/utf8_iterator.o src/utf8_utils.o
CLI_OBJ=$(COMMON_OBJ) src/cli.o
TEST_OBJ=$(COMMON_OBJ) test/test.o
HDR=\
	src/akutf.h \
	src/utf8_constants.h \
	src/utf8_decode.h \
	src/utf8_string.h \
	src/utf8_iterator.h \
	src/utf8_utils.h

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

$(COMMON_OBJ) cli.o test/test.o: $(HDR)

clean:
	rm -f cli tests $(COMMON_OBJ) src/cli.o test/test.o
