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
		int numThreads; 	// The number of threads
		bool shutdown;
		std::vector<std::shared_ptr<std::thread>> myThreads;
		SynchronousQueue<Task> myTasks;
		std::once_flag onceFlag;

		void run() {
			while (!shutdown) {
				Task task;
				if (myTasks.take(task)) {
					task();
				}
			}
		}

	public:
		ThreadPool(int _numThreads = std::thread::hardware_concurrency(), int maxTask = 100) 
		: myTasks(maxTask), shutdown(false) {
			for (int i = 0; i < _numThreads; ++i) {
				myThreads.push_back(std::make_shared<std::thread>([this] { run(); } ));
			}
		}
		
		~ThreadPool() {
			shutdownNow();
		}
		
		void submit(const Task& task) {
			myTasks.put(task);
		}
		
		void submit(Task&& task) {
			myTasks.put(std::forward<Task>(task));
		}
		
		void shutdownNow() {
			std::call_once(onceFlag, [this]{
				shutdown = true;
				myTasks.stop();	
				for (auto thread : myThreads) {
					if (thread)
						thread->join();
				}
				myThreads.clear();
			});
		}

		void setMaxTask(int maxTask) {
			myTasks.setMaxSize(maxTask);	
		}
	};
	
}

#endif
