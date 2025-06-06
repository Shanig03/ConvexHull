CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread -g
INCLUDES = -I.

# Target executables
TARGETS = convex_hull_server convex_hull_client reactor_test original_server original_client

# Source files
COMMON_SOURCES = convex_hull.cpp
SERVER_SOURCES = convex_hull_reactor.cpp $(COMMON_SOURCES)
CLIENT_SOURCES = client.cpp $(COMMON_SOURCES)

# Header files
HEADERS = reactor.hpp convex_hull.hpp

.PHONY: all clean test run-server run-client demo help

all: $(TARGETS)

# Build reactor-based convex hull server
convex_hull_server: $(SERVER_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SERVER_SOURCES)

# Build enhanced convex hull client
convex_hull_client: $(CLIENT_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CLIENT_SOURCES)

# Clean build artifacts
clean:
	rm -f $(TARGETS) *.o *~ core

# Run server
run-server: convex_hull_server
	./convex_hull_server

# Run client
run-client: convex_hull_client
	./convex_hull_client