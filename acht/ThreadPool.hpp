#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include "SyncQueue.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace acht {

    class ThreadPool {
    private:
        using Task = std::function<void()>;
        std::atomic<bool> shutdown;
        std::vector<std::shared_ptr<std::thread>> my_threads;
        SyncQueue<Task> my_tasks;

        /***********************************************************
         *  Run the thread until the pool is shut down.
         *  Execute every task it takes from the task queue, waiting
         *  if the task queue is empty.
         ***********************************************************/
        void run() {
            while (!shutdown) {
                Task task;
                if (my_tasks.take(task)) {
                    task();
                }
            }
        }

        /***********************************************************
         *  Create worker threads.
         ***********************************************************/
        void makeThreads(int thread_num) {
            for (int i = 0; i < thread_num; ++i) {
                my_threads.push_back(std::make_shared<std::thread>([this] {
                    run();
                }));
            }
        }

    public:
        /***********************************************************
         *  Create a thread pool. Set the number of threads and max
         *  tasks number.
         ***********************************************************/
        ThreadPool(int thread_num = std::thread::hardware_concurrency(), int maxTask = 100)
        : my_tasks(maxTask), shutdown(false) {
            makeThreads(thread_num);
        }

        /***********************************************************
         *  Shut down the pool if it wasn't shut down.
         ***********************************************************/
        ~ThreadPool() {
            shutdownNow();
        }

        /***********************************************************
         *  Submit task(lvalue) to the task queue.
         ***********************************************************/
        void submit(const Task& task) {
            my_tasks.put(task);
        }

        /***********************************************************
         *  Submit task(rvalue) to the task queue.
         ***********************************************************/
        void submit(Task&& task) {
            my_tasks.put(std::forward<Task>(task));
        }

        /***********************************************************
         *  If the pool was shut down, restart it.
         ***********************************************************/
        void start(int thread_num = std::thread::hardware_concurrency(), int maxTask = 100) {
            if (shutdown) {
                shutdown = false;
                makeThreads(thread_num);
                // Restart the task queue
                my_tasks.start();
                my_tasks.setMaxSize(maxTask);
            }
        }

        /***********************************************************
         *  Shut down the pool.
         ***********************************************************/
        void shutdownNow() {
            if (!shutdown) {
                shutdown = true;

                // Stop the task queue
                my_tasks.stop();

                // Wait until submitted tasks are finish
                for (auto thread : my_threads) {
                    if (thread) {
                        thread->join();
                    }
                }
                my_threads.clear();
            }
        }

        /***********************************************************
         *  Set max tasks number.
         ***********************************************************/
        void setMaxTask(int maxTask) {
            my_tasks.setMaxSize(maxTask);
        }
    };

}

#endif