#ifndef REACTOR_HPP
#define REACTOR_HPP

// Function pointer type definition
typedef void* (*reactorFunc)(int fd);

// Starts new reactor and returns pointer to it
void* startReactor();

// Adds fd to reactor (for reading); returns 0 on success
int addFdToReactor(void* reactor, int fd, reactorFunc func);

// Removes fd from reactor
int removeFdFromReactor(void* reactor, int fd);

// Stops reactor
int stopReactor(void* reactor);

// Run one iteration of the reactor event loop
int runReactorOnce(void* reactor);

#endif