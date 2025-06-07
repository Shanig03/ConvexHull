CC=g++
CFLAGS=-std=c++11 -Wall -Wextra -O2 -pthread -g

# Default target
all: convex_hull client

# Server compilation
convex_hull: convex_hull.cpp convex_hull.hpp
	$(CC) $(CFLAGS) -o convex_hull convex_hull.cpp

# Client compilation  
client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp

# Clean compiled files
clean:
	rm -f convex_hull client

.PHONY: all clean