#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <map>
#include <set>
#include <thread>
#include <atomic>
#include <sys/select.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

// Function pointer typedef as specified
typedef void* (*reactorFunc)(int fd);

// Template reactor class
template<typename FdType = int>
class Reactor {
private:
    std::map<FdType, reactorFunc> fdFunctions;  // Maps fd to its callback function
    std::set<FdType> activeFds;                 // Set of active file descriptors
    std::atomic<bool> running;                  // Reactor running state
    std::thread reactorThread;                  // Thread for reactor loop
    int maxFd;                                  // Maximum file descriptor value

    // Main reactor loop
    void reactorLoop() {
        fd_set readfds;
        struct timeval tv;
        
        while (running.load()) {
            // Initialize file descriptor set
            FD_ZERO(&readfds);
            maxFd = 0;
            
            // Add all active file descriptors to the set
            for (const auto& fd : activeFds) {
                FD_SET(fd, &readfds);
                if (fd > maxFd) {
                    maxFd = fd;
                }
            }
            
            // Set timeout
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            // Wait for activity on file descriptors
            int activity = select(maxFd + 1, &readfds, NULL, NULL, &tv);
            
            if (activity < 0) {
                if (running.load()) {
                    perror("select error in reactor");
                }
                continue;
            }
            
            // Check each file descriptor for activity
            for (const auto& fd : activeFds) {
                if (FD_ISSET(fd, &readfds)) {
                    // Call the associated function
                    auto it = fdFunctions.find(fd);
                    if (it != fdFunctions.end() && it->second != nullptr) {
                        it->second(fd);
                    }
                }
            }
        }
    }

public:
    Reactor() : running(false), maxFd(0) {}
    
    ~Reactor() {
        stop();
    }
    
    // Start the reactor
    bool start() {
        if (running.load()) {
            return false; // Already running
        }
        
        running.store(true);
        reactorThread = std::thread(&Reactor::reactorLoop, this);
        return true;
    }
    
    // Stop the reactor
    bool stop() {
        if (!running.load()) {
            return false; // Already stopped
        }
        
        running.store(false);
        if (reactorThread.joinable()) {
            reactorThread.join();
        }
        return true;
    }
    
    // Add file descriptor to reactor
    int addFd(FdType fd, reactorFunc func) {
        if (activeFds.find(fd) != activeFds.end()) {
            return -1; // FD already exists
        }
        
        activeFds.insert(fd);
        fdFunctions[fd] = func;
        return 0; // Success
    }
    
    // Remove file descriptor from reactor
    int removeFd(FdType fd) {
        auto it = activeFds.find(fd);
        if (it == activeFds.end()) {
            return -1; // FD doesn't exist
        }
        
        activeFds.erase(it);
        fdFunctions.erase(fd);
        return 0; // Success
    }
    
    // Get number of active file descriptors
    size_t getActiveFdCount() const {
        return activeFds.size();
    }
    
    // Check if reactor is running
    bool isRunning() const {
        return running.load();
    }

    bool runOnce() {
        if (!running.load()) {
            return false;
        }

        fd_set readfds;
        struct timeval tv;
        
        // Initialize file descriptor set
        FD_ZERO(&readfds);
        maxFd = 0;
        
        // Add all active file descriptors to the set
        for (const auto& fd : activeFds) {
            FD_SET(fd, &readfds);
            if (fd > maxFd) {
                maxFd = fd;
            }
        }
        
        // Set timeout
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        // Wait for activity on file descriptors
        int activity = select(maxFd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity < 0) {
            if (running.load()) {
                perror("select error in reactor");
            }
            return false;
        }
        
        // Check each file descriptor for activity
        for (const auto& fd : activeFds) {
            if (FD_ISSET(fd, &readfds)) {
                // Call the associated function
                auto it = fdFunctions.find(fd);
                if (it != fdFunctions.end() && it->second != nullptr) {
                    it->second(fd);
                }
            }
        }
        
        return true;
    }
};

// C-style interface functions as specified in the requirements
typedef Reactor<int> reactor;

// Start new reactor and return pointer to it
void* startReactor() {
    reactor* r = new reactor();
    if (r->start()) {
        return static_cast<void*>(r);
    } else {
        delete r;
        return nullptr;
    }
}

// Add fd to Reactor (for reading); returns 0 on success
int addFdToReactor(void* reactorPtr, int fd, reactorFunc func) {
    if (reactorPtr == nullptr) {
        return -1;
    }
    
    reactor* r = static_cast<reactor*>(reactorPtr);
    return r->addFd(fd, func);
}

// Remove fd from reactor
int removeFdFromReactor(void* reactorPtr, int fd) {
    if (reactorPtr == nullptr) {
        return -1;
    }
    
    reactor* r = static_cast<reactor*>(reactorPtr);
    return r->removeFd(fd);
}

// Stop reactor
int stopReactor(void* reactorPtr) {
    if (reactorPtr == nullptr) {
        return -1;
    }
    
    reactor* r = static_cast<reactor*>(reactorPtr);
    bool success = r->stop();
    delete r;
    return success ? 0 : -1;
}





#endif // REACTOR_HPP