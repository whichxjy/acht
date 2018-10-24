#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include "SynchronousQueue.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <functional>

namespace acht {
	class ThreadPool {
	private:
		int numThreads; 	// The number of threads
		bool shutdown;
		std::vector<std::shared_ptr<std::thread>> myThreads;
		SynchronousQueue<Task> myTasks;

		void run() {
			while (!shutdown) {
				Task task;
				myTasks.take(task);
				// Execute
				task();
			}
		}
		
	public:
		using Task = std::function<void()>;
		ThreadPool(int _numThreads) : myTasks(_numThreads), shutdown(true) {
			for (int i = 0; i < _numThreads; ++i) {
				myThreads.push_back(std::make_shared<std::thread>([this] {run()} ));
			}
		}
		
		~ThreadPool() {
			shutdown();
		}
		
		void shutdown() {
			shutdown = true;
			myTasks.stop();
			
			for (auto thread : myThreads) {
				if (thread)
					thread->join();
			}
			
			myThreads.clear();
		}
		
		void submit(const Task& task) {
			myTasks.put(task);
		}
		
		void submit(Task&& task) {
			myTasks.put(std::forward<Task>(task));
		}
		
		void setMaxTask(int maxTask) {
			myTasks.setMaxSize(maxTask);	
		}
	};
	
}

#endif
