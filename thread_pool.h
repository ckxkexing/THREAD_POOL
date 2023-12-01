#pragma once

#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include "block_queue.cpp"


class ThreadPool
{
private:
    class ThreadWorker // 内置线程工作类
    {
        private:
            int m_id; // 工作id
            ThreadPool *m_pool;  // 所属线程池

        public:
            ThreadWorker(ThreadPool *pool, const int id) : m_pool(pool), m_id(id){

            }

            void operator()(){
                std::function<void()> func;

                while(!m_pool->m_shutdown) {
                    {
                        std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
                        while(m_pool->m_queue.empty()) {
                            m_pool->m_conditional_lock.wait(lock);
                            if(m_pool->m_shutdown) {
                                return;
                            }
                        }
                        func = m_pool->m_queue.pop();
                    }
                    func();
                }
            }
    };

    bool m_shutdown; 

    BlockQueue<std::function<void()>> m_queue; // 执行函数安全队列，即任务队列

    std::vector<std::thread> m_threads;

    std::mutex m_conditional_mutex;

    std::condition_variable m_conditional_lock;

public:
    ThreadPool(const int n_threads=4) 
        : m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false)
    {
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void init(){
        for(int i=0; i<m_threads.size(); i++) {
            m_threads.at(i) = std::thread(ThreadWorker(this, i));
        }
    }

    void shutdown(){
        m_shutdown = true;
        m_conditional_lock.notify_all(); // 通知唤醒所有工作线程

        for(int i=0; i<m_threads.size(); i++) {
            if(m_threads.at(i).joinable()) {
                m_threads.at(i).join();
            }
        }
    }

    ~ThreadPool(){
        if(m_shutdown == false)
            shutdown();
    }

    // submit a function to be executed asynchronously by the pool
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // 将task_ptr指向的函数取出，包装为void函数。
        std::function<void()> warpper_func = [task_ptr]()
        {
            (*task_ptr)();
        };

        m_queue.push_back(warpper_func);

        m_conditional_lock.notify_one();

        return task_ptr->get_future();
    }
};
