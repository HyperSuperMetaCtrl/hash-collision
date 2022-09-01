#ifndef THREADPOOLH
#define THREADPOOLH

#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>

class Threadpool {
    public:
        //creates an unitialized Threadpool
        Threadpool();
        ~Threadpool(); 
        //allows to add functions to the task queue
        void queueTask(std::function<void()> task);
        //syncs and waits for queue to be empty and joins threads
        void shutdown();
        void waitUntilEmpty();
        std::vector<std::thread> threads;
        // the workerfunction threads are running
        void workloop();
        void reset();
    private:

        std::mutex queueMutex;
        std::deque<std::function<void()>> taskQueue;
        std::mutex vectorMutex;
        std::condition_variable condition;
        bool terminated;
        bool stopped;

};
#endif 
