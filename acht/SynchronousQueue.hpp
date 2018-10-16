#ifndef _SYNCHRONOUS_QUEUE_HPP_
#define _SYNCHRONOUS_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>

namespace acht {
	template <typename T>
	class SynchronousQueue {
	private:
		std::queue<T> myQueue;
		int queueMaxSize;
		std::mutex myMutex;
		std::condition_variable notEmpty;
		std::condition_variable notFull;

	public:
		SynchronousQueue(int maxSize = 999999999) : queueMaxSize(maxSize) {
		}

		~SynchronousQueue() = default;

		// No copy
		SynchronousQueue(const SynchronousQueue&) = delete;
		
		// No assignment
		SynchronousQueue& operator=(const SynchronousQueue&) = delete;

		void Put(T msg) {
			std::lock_guard<std::mutex> lock(myMutex);
			while(IsFull()) {
				notFull.wait(myMutex);
			}
			myQueue.push(msg);
			notEmpty.notify_one;
		}		

		bool Take(T& msg, bool Blocked = true) {
			std::lock_guard<std::mutex> lock(myMutex);
			if (Blocked) {
				while (IsEmpty()) {
				    notEmpty.wait(myMutex);
				}
			}
			else {
				if (IsEmpty())
					return false;
			}
			msg = myQueue.front();
			myQueue.pop();
			return true;
		}

		int getSize() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size();
		}

		bool IsFull() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size() == queueMaxSize;
		}

		bool IsEmpty() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size() == 0;
		}
		
		int GetMaxSize() const {
			return queueMaxSize;
		}
		
		void SetMaxSize(int maxSize) {
			std::lock_guard<std::mutex> lock(myMutex);
			queueMaxSize = maxSize;
		}

	}
}

#endif
