CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg
INCLUDES = -I.

TARGET = performance_test
SOURCES = performance_test.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) gmon.out

# For profiling run the program with:
# make
# then:
# ./performance_test
# After running, you can analyze the profiling data with:
# gprof performance_test gmon.out > analysis.txt