# Makefile for Reactor Template Library

CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
INCLUDES = -I.

# Target executables
TARGETS = reactor_server reactor_test

# Source files
REACTOR_SERVER_SOURCES = reactor_example.cpp
REACTOR_TEST_SOURCES = test_reactor.cpp

# Header files
HEADERS = reactor.hpp

.PHONY: all clean test

all: $(TARGETS)

# Build reactor server example
reactor_server: $(REACTOR_SERVER_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(REACTOR_SERVER_SOURCES)

# Build reactor test program
reactor_test: $(REACTOR_TEST_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(REACTOR_TEST_SOURCES)

# Run tests
test: reactor_test
	./reactor_test

# Clean build artifacts
clean:
	rm -f $(TARGETS) *.o *~

# Install (copy headers to system include directory)
install: $(HEADERS)
	sudo cp $(HEADERS) /usr/local/include/

# Uninstall
uninstall:
	sudo rm -f /usr/local/include/reactor.hpp

