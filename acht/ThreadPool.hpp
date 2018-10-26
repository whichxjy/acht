#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include "SynchronousQueue.hpp"
#include <vector>
#include <memory>
#include <thread>

namespace acht {
	class ThreadPool {
	private:
		using Task = std::function<void()>;
		bool shutdown;
		std::vector<std::shared_ptr<std::thread>> myThreads;
		SynchronousQueue<Task> myTasks;
		std::once_flag onceFlag; // It's used as an argument for call_once.
		
		/***********************************************************
		 *  Run the thread until the pool is shut down. 
		 *  Execute every task it takes from the task queue, waiting
		 *  if the task queue is empty.
		 ***********************************************************/
		void run() {
			while (!shutdown) {
				Task task;
				if (myTasks.take(task)) {
					task();
				}
			}
		}

	public:
		/***********************************************************
		 *  Create a thread pool. Set the number of threads and max
		 *  tasks number.
		 ***********************************************************/
		ThreadPool(int numThreads = std::thread::hardware_concurrency(), int maxTask = 100) 
		: myTasks(maxTask), shutdown(false) {
			// Create worker threads
			for (int i = 0; i < numThreads; ++i) {
				myThreads.push_back(std::make_shared<std::thread>([this] { run(); } ));
			}
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
			myTasks.put(task);
		}
		
		/***********************************************************
		 *  Submit task(rvalue) to the task queue.
		 ***********************************************************/
		void submit(Task&& task) {
			myTasks.put(std::forward<Task>(task));
		}
		
		/***********************************************************
		 *  Shut down the pool.
		 ***********************************************************/
		void shutdownNow() {
			// Avoid calling more than once
			std::call_once(onceFlag, [this]{
				shutdown = true;
				
				// Stop the task queue
				myTasks.stop();	
				
				// Wait until submitted tasks are finish
				for (auto thread : myThreads) {
					if (thread)
						thread->join();
				}
				myThreads.clear();
			});
		}

		/***********************************************************
		 *  Set max tasks number.
		 ***********************************************************/
		void setMaxTask(int maxTask) {
			myTasks.setMaxSize(maxTask);	
		}
	};
	
}

#endif
