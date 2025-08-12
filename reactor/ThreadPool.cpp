#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadnum,const std::string& threadtype):stop_(false),threadtype_(threadtype)
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上。
    for(size_t ii = 0; ii < threadnum;ii++)
    {
        // 用lambda函创建线程。
        threads_.emplace_back([this]
        {
            printf("create %s thread(%ld).\n",threadtype_.c_str(),syscall(SYS_gettid));

            while(stop_ == false)
            {
                std::function<void()> task;       // 用于存放出队的元素。

                {   // 锁作用域的开始。 ///////////////////////////////////
                    std::unique_lock<std::mutex> lock(mutex_);

                    // 等待生产者的条件变量。
                    condition_.wait(lock,[this]{
                        return ((stop_ == true) || (taskqueue_.empty() == false));
                    });

                    if((stop_ == true) && (taskqueue_.empty() == true)) return;

                    task = move(taskqueue_.front());
                    taskqueue_.pop();
                }

                //printf("%s(%ld) execute task.\n",threadtype_.c_str(),syscall(SYS_gettid));
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    Stop();
}

void ThreadPool::Stop()
{
    if(stop_) return;

    stop_ = true;
    condition_.notify_all();

    // 等待全部线程执行完任务后退出。
    for(auto &th:threads_)
    {
        th.join();
    }  
}

void ThreadPool::addtask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }

    condition_.notify_one();
}

size_t ThreadPool::size()
{
    return threads_.size();
}