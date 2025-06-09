CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g -pthread
INCLUDES = -I.

TARGET = reactor_proactor_test

SOURCES = reactor_proactor.cpp test_reactor.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = reactor_proactor.hpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJECTS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) *~