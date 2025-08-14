#include "reactor_proactor.hpp"
#include <unordered_set>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_set>

#define MAX_FD 1024

proactorFunc globalProactorFunc = nullptr;

struct reactorStruct {
    std::unordered_set<int> fds; // Set of active file descriptors
    reactorFunc funcs[MAX_FD] = {nullptr}; // Callback functions per fd
    bool running = false; // Whether the loop is running
};

void* startReactor() {
    reactorStruct* r = new reactorStruct;
    r->running = true;  // Start running immediately
    return static_cast<void*>(r);
}

int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    if (!reactor || fd < 0 || fd >= MAX_FD) return -1;
    auto r = static_cast<reactorStruct*>(reactor);
    r->fds.insert(fd);
    r->funcs[fd] = func;
    return 0;
}

int removeFdFromReactor(void* reactor, int fd) {
    if (!reactor || fd < 0 || fd >= MAX_FD) return -1;
    auto r = static_cast<reactorStruct*>(reactor);
    r->fds.erase(fd);
    r->funcs[fd] = nullptr;
    return 0;
}

int stopReactor(void* reactor) {
    if (!reactor) return -1;
    auto r = static_cast<reactorStruct*>(reactor);
    r->running = false;
    delete r;
    return 0;
}

// Run one iteration of the reactor event loop
int runReactorOnce(void* reactor) {
    if (!reactor) return -1;
    auto r = static_cast<reactorStruct*>(reactor);
    
    if (!r->running) return -1;
    
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    int maxfd = 0;

    for (int fd : r->fds) {
        FD_SET(fd, &readfds);
        if (fd > maxfd) maxfd = fd;
    }

    tv.tv_sec = 0;  // Don't block for too long
    tv.tv_usec = 100000;  // 100ms timeout

    int ready = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
    if (ready < 0) {
        if (errno == EINTR) return 0;
        perror("select");
        return -1;
    }

    auto fds = r->fds;  // Copy to avoid iterator invalidation
    for (int fd : fds) {
        if (FD_ISSET(fd, &readfds)) {
            if (r->funcs[fd]) {
                r->funcs[fd](fd);
            }
        }
    }

    return 0;
}



// Proactor additions

// Proactor structure to hold arguments for the proactor thread
struct proactorArgs {
    int sockfd;
    proactorFunc threadFunc;
    bool running;
};

// Helper to call proactorFunc with int argument
static void* proactorThreadWrapper(void* arg) {
    // arg is actually the clientfd cast to void*
    int clientfd = (int)(intptr_t)arg;
    extern proactorFunc globalProactorFunc; // Forward declaration
    return globalProactorFunc(clientfd);
}

// Start the proactor main loop in a separate thread
static void* proactorMain(void* arg) {
    proactorArgs* args = static_cast<proactorArgs*>(arg);
    int listenfd = args->sockfd;
    proactorFunc func = args->threadFunc;
    globalProactorFunc = func; 

    while (args->running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientfd = accept(listenfd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientfd < 0) {
            if (args->running) perror("accept");
            continue;
        }
        // Create a new thread to handle the client
        pthread_t tid;
        int ret = pthread_create(&tid, nullptr, proactorThreadWrapper, (void*)(intptr_t)clientfd);
        if (ret != 0) {
            perror("pthread_create");
            close(clientfd);
        } else {
            pthread_detach(tid); // Detach so resources are freed when thread exits
        }
    }
    delete args;
    return nullptr;
}

// Start the proactor thread
pthread_t startProactor(int sockfd, proactorFunc threadFunc) {
    pthread_t tid;
    proactorArgs* args = new proactorArgs{sockfd, threadFunc, true};
    if (pthread_create(&tid, nullptr, proactorMain, args) != 0) {
        perror("pthread_create (proactor)");
        delete args;
        return 0;
    }
    return tid;
}

// Stop the proactor thread
int stopProactor(pthread_t tid) {
    return pthread_cancel(tid);
}