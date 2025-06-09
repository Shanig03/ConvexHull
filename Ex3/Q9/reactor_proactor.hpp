#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <pthread.h> 

// Reactor API
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



// Proactor additions
typedef void* (*proactorFunc)(int sockfd);

// Starts new proactor and returns proactor thread id
pthread_t startProactor(int sockfd, proactorFunc threadFunc);

// Stops proactor by thread id
int stopProactor(pthread_t tid);


#endif