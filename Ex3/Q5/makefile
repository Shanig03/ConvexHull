CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O
INCLUDES = -I.

# Target executable
TARGET = reactor_test

# Source files
SOURCES = reactor.cpp test_reactor.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Header files
HEADERS = reactor.hpp

.PHONY: all clean run

all: $(TARGET)

# Build reactor test executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Compile source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the tests
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS) *~