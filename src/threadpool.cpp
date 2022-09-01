#include "threadpool.h"
Threadpool::Threadpool() 
{
    terminated = false;
    stopped = false;
};
Threadpool::~Threadpool()
{
    if (!stopped) shutdown();
    else return;
}
void Threadpool::queueTask(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> guard(queueMutex);
        this->taskQueue.push_back(task);
    }
    condition.notify_one();

};
void Threadpool::reset()
{
    stopped = false;
    terminated = false;
}
void Threadpool::workloop()
{   std::function<void()> task;
    while(true)
    {
        {
            std::unique_lock<std::mutex> guard(this->queueMutex);

            condition.wait(guard, [&]{return !taskQueue.empty() || terminated;});
            if (terminated && taskQueue.empty()) return;
            if (!taskQueue.empty()) {
            task = taskQueue.front();
            taskQueue.pop_front();}
        }
        task();
    }
};
void Threadpool::shutdown() {
    { std::unique_lock<std::mutex> guard(vectorMutex);
    terminated = true; }

    condition.notify_all();
    for (std::thread &th : threads){
        th.join();
    }
    threads.clear();
    stopped = true;
};
