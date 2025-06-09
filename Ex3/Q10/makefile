CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g -pthread
INCLUDES = -I.

SERVER_TARGET = convex_hull_server
CLIENT_TARGET = convex_hull_client

SERVER_SOURCES = convex_hull.cpp reactor_proactor.cpp
CLIENT_SOURCES = client.cpp

HEADERS = convex_hull.hpp reactor_proactor.hpp

.PHONY: all clean

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SERVER_SOURCES)

$(CLIENT_TARGET): $(CLIENT_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CLIENT_SOURCES)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET) *.o *~