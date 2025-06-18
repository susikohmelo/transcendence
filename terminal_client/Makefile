HOSTNAME = "localhost:8080/ws"

PROGRAM = clipong
TENGILIB = tengine
TENGILIB_DIR = tengine/

# FOR DOCKER COMPILATION
INCLUDE_DIRS=-I$(HOME)/lib/boost/boost_1_67_0/include/ -I$(HOME)/src/websocketpp/websocketpp-0.8.1 -Ijson/include
LIB_FLAGS=-lpthread -Llib -L$(HOME)/lib/boost/boost_1_67_0/lib -lboost_system -lssl -lcrypto -Llib -lX11 -lm -static-libgcc

CPP	= g++
CC	= gcc

HEADERS = $(wildcard include/*.h) $(wildcard include/*.hpp) $(wildcard $(TENGILIB_DIR)*.h)
CPP_SRCS = $(wildcard src/*.cpp)
CPP_OBJS = $(patsubst src/%.cpp, build/%.o, $(CPP_SRCS))
C_SRCS = $(wildcard src/*.c)
C_OBJS = $(patsubst src/%.c, build/%.o, $(C_SRCS))

EXTRA_CFLAGS = -DUSER_SCALAR_TYPE=float # this is also passed to tengine
CFLAGS = -Wall -Wextra -Werror -Iinclude -I$(TENGILIB_DIR)include -fopenmp $(EXTRA_CFLAGS)
CPPFLAGS = -std=c++17 $(CFLAGS)
DEBUG_FLAGS = -ggdb3 -gdwarf -fsanitize=address -fsanitize=leak -fsanitize=undefined -static-libasan -static-libubsan -no-pie
RELEASE_FLAGS = -O3 -march=native -fno-math-errno
LFLAGS = -lX11 -lm -lpthread -lOpenCL

all: CFLAGS += $(RELEASE_FLAGS)
all: $(CPP_OBJS) $(C_OBJS)
	cd $(TENGILIB_DIR) && make EXTRA_CFLAGS=$(EXTRA_CFLAGS)
	$(CPP) -o $(PROGRAM) $(CPPFLAGS) $(CPP_OBJS) $(C_OBJS) -L$(TENGILIB_DIR)/build -l$(TENGILIB) $(LIB_FLAGS) $(INCLUDE_DIRS) $(LFLAGS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(CPP_OBJS) $(C_OBJS)
	cd $(TENGILIB_DIR) && make debug EXTRA_CFLAGS=$(EXTRA_CFLAGS)
	$(CPP) -o $(PROGRAM) $(CPPFLAGS) $(CPP_OBJS) $(C_OBJS) -L$(TENGILIB_DIR)/build -l$(TENGILIB) $(LIB_FLAGS) $(INCLUDE_DIRS) $(LFLAGS)

install_json: json/

json/:
	git clone https://github.com/nlohmann/json.git
	cd json && rm -rf build
	cd json && mkdir build ; cd build && cmake ../ && make && sudo make install

run:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:lib ./$(PROGRAM) $(HOSTNAME)

docker_compile:
	cd docker && docker compose down --rmi all -v && PWD=$(PWD) MODE=all docker compose up --build --force-recreate

docker_debug:
	cd docker && docker compose down --rmi all -v && PWD=$(PWD) MODE=debug docker compose up --build --force-recreate

build/%.o: src/%.cpp $(HEADERS)
	mkdir -p build
	$(CPP) -c -o $@ $< $(CPPFLAGS) $(INCLUDE_DIRS) $(LIB_FLAGS) $(LFLAGS)

build/%.o: src/%.c $(HEADERS)
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) $(LFLAGS)

clean:
	cd $(TENGILIB_DIR) && make clean
	rm -rf build

fclean: clean
	rm -rf $(PROGRAM)
	rm -rf lib
	rm -rf json

.PHONY: clean fclean
