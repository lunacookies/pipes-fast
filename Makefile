CC=clang

CFLAGS=\
	-std=c11 \
	-O3 \
	-flto \
	-W \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wimplicit-fallthrough \
	-Wimplicit-int-conversion \
	-Wshadow \
	-Wstrict-prototypes \
	-Wmissing-prototypes

HEADERS=$(wildcard *.h)
SOURCES=$(wildcard *.c)
OBJECTS=$(addprefix out/, $(SOURCES:.c=.o))

all: out/pipes-fast tidy

out/pipes-fast: $(OBJECTS)
	@ mkdir -p out
	@ $(CC) $(CFLAGS) $^ -o $@

out/%.o: %.c $(HEADERS)
	@ mkdir -p out
	@ $(CC) $(CFLAGS) -c -o $@ $<

bench: all
	@ ./out/pipes-fast --bench

tidy: $(HEADERS) $(SOURCES)
	@ clang-format -i $^

clean:
	@ rm -r out

.PHONY: all test tidy clean

