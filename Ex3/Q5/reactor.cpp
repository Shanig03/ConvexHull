#include "reactor.hpp"
#include <unordered_set>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define MAX_FD 1024

struct reactorStruct {
    std::unordered_set<int> fds;            // Set of active file descriptors
    reactorFunc funcs[MAX_FD] = {nullptr};  // Callback functions per fd
    bool running = false;                    // Whether the loop is running
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