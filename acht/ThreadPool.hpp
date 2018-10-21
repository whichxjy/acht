#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include "acht/SynchronousQueue.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <functional>

namespace acht {
	template<typename Args...>
	class ThreadPool {
	private:
		using Task = std::function<void(Args...)>;
		int numThreads; 	// The number of threads
		bool shutdown;
		std::vector<std::shared_ptr<std::thread>> myThreads;
		acht::SynchronousQueue<Task> myTasks;


	public:

	};
	
}

#endif
