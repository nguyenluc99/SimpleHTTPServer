SRC_DIR=./src
CC=g++
CXXFLAGS=

# SOURCES = server.cc client.cc
# OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

server:
	$(CC) $(CXXFLAGS) $(SRC_DIR)/server.cc -o server
client:
	$(CC) $(CXXFLAGS) $(SRC_DIR)/client.cc -o client

all: server client

clean:
	rm -rf server client