CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g
INCLUDES = -I.

TARGETS = convex_hull_server convex_hull_client

SERVER_SOURCES = convex_hull.cpp reactor.cpp
CLIENT_SOURCES = client.cpp

HEADERS = reactor.hpp convex_hull.hpp

.PHONY: all clean run-server run-client

all: $(TARGETS)

convex_hull_server: $(SERVER_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SERVER_SOURCES)

convex_hull_client: $(CLIENT_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CLIENT_SOURCES)

clean:
	rm -f $(TARGETS) *.o *~ core

run-server: convex_hull_server
	./convex_hull_server

run-client: convex_hull_client
	./convex_hull_client


# The following lines are for building a static library

#libreactor.a: reactor.cpp reactor.hpp
#	$(CXX) $(CXXFLAGS) -c reactor.cpp && ar rcs libreactor.a reactor.o

#convex_hull_server: convex_hull.cpp libreactor.a
#	$(CXX) $(CXXFLAGS) -o convex_hull_server convex_hull.cpp libreactor.a
