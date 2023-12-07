#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

// Thread safe implementation of a Queue using a std::queue
template <typename T>
class SafeQueue
{
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
public:
    SafeQueue() {}
    SafeQueue(SafeQueue &&other) {}
    ~SafeQueue() {}
    bool empty()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    int size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    void enqueue(T &t)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(t);
    }
    bool dequeue(T &t)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;
        t = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }
};

class ThreadPool{

public:
    ThreadPool(const int n_threads = 4)
        : workers(std::vector<std::thread>(n_threads)), isShutdown(false)
    {
    }
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    
    // Submit a function to be executed asynchronously by the pool
    template <typename F, typename... Args>
    void submit(F &&f, Args &&...args)
    {
        // Create a function with bounded parameter ready to execute
         // 连接函数和参数定义，特殊函数类型，避免左右值错误
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // Encapsulate it into a shared pointer in order to be able to copy construct
        auto task_ptr = std::make_shared<decltype(func)>(func);
        // Warp packaged task into void function
        std::function<void()> warpper_func = [task_ptr]()
        {
            (*task_ptr)();
        };
        taskQueue.enqueue(warpper_func);
        conditional.notify_one();
    }

    void init()
    {
        auto executor = [this](){
                std::function<void()> func; 
                bool dequeued; 
                // 当线程池结束了就应该停止运行，但是依然需要把任务队列里剩余的工作做完
                while ((!isShutdown) || (!taskQueue.empty()))
                {
                    {
                        std::unique_lock<std::mutex> lock(m_conditional_mutex);
                        // 当线程池没有结束，并且任务队列为空的话，进入阻塞等待状态
                        while ((taskQueue.empty()) && (!isShutdown))
                        {
                            conditional.wait(lock);
                        }
                        dequeued = taskQueue.dequeue(func);
                    }
                    if (dequeued) {
                        func();
                    }
                }
            };

        for (int i = 0; i < workers.size(); ++i)
        {
            workers.at(i) = std::thread(executor);
        }
    }
    void shutdown()
    {
        isShutdown = true;
        conditional.notify_all();
        for (int i = 0; i < workers.size(); ++i)
        {
            if (workers.at(i).joinable())
            {
                workers.at(i).join();
            }
        }
    }

private:

    bool isShutdown;
    SafeQueue<std::function<void()>> taskQueue;
    std::vector<std::thread> workers;
    std::mutex m_conditional_mutex;
    std::condition_variable conditional;

};


#endif
