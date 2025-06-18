CC = cc

SRCS = $(wildcard src/*)
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))
CFLAGS = -Wall -Wextra -Iinclude $(EXTRA_CFLAGS)
DEBUG_FLAGS = -ggdb3 -gdwarf -fsanitize=address -fsanitize=leak -fsanitize=undefined
RELEASE_FLAGS = -O3 -flto -march=native -fno-math-errno
LFLAGS = -lX11 -lm

all: CFLAGS += $(RELEASE_FLAGS)
all: $(OBJS)
	ar -rcs build/libtengine.a $(OBJS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(OBJS)
	ar -rcs build/libtengine.a $(OBJS)

build/%.o: src/%.c $(wildcard include/*.h)
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf build
