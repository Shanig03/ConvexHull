CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
INCLUDES = -I. # Include current directory for headers

# Target executable
TARGET = convex_hull

# Source files
SOURCES = convex_hull.cpp

# Header files
HEADERS = convex_hull.hpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

# Build convex hull executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Compile source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS) *~