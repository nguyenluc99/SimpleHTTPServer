SRC_DIR=./src
CC=g++
CXXFLAGS=-g
LIBS=-lev -lpthread -pthread -fsplit-stack

# SOURCES = server.cc client.cc
# OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

all: server main

server:
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/server.cc $(LIBS)

main:
	$(CC) $(CXXFLAGS) server.o $(SRC_DIR)/main.cc -o main  $(LIBS)
clean:
	rm -rf server.o main
