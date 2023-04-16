SRC_DIR=./src
CC=g++
CXXFLAGS=
LIBS=-pthread

# SOURCES = server.cc client.cc
# OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

all: server main

server:
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/$@.cc $(LIBS) -o $(SRC_DIR)/$@

main: server
	$(CC) $(CXXFLAGS) $(SRC_DIR)/$< $(SRC_DIR)/$@.cc -o $@  $(LIBS)
clean:
	rm -rf $(SRC_DIR)/server main
