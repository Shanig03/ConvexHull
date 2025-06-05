CC=g++
CFLAGS=-std=c++11 -Wall -Wextra -O2

# Target executables
SERVER=convex_hull
CLIENT=client

# Default target
all: $(SERVER) $(CLIENT)

# Server compilation
$(SERVER): convex_hull.cpp
	$(CC) $(CFLAGS) -o $(SERVER) convex_hull.cpp

# Client compilation  
$(CLIENT): client.cpp
	$(CC) $(CFLAGS) -o $(CLIENT) client.cpp

# Clean compiled files
clean:
	rm -f $(SERVER) $(CLIENT)

# Install (copy to /usr/local/bin)
install: all
	sudo cp $(SERVER) /usr/local/bin/
	sudo cp $(CLIENT) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(SERVER)
	sudo rm -f /usr/local/bin/$(CLIENT)

.PHONY: all clean install uninstall