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
		SynchronousQueue(int maxSize = 9999) : queueMaxSize(maxSize) {
		}

		~SynchronousQueue() = default;

		// No copy
		SynchronousQueue(const SynchronousQueue&) = delete;
		
		// No assignment
		SynchronousQueue& operator=(const SynchronousQueue&) = delete;
		
		/***********************************************************
		 *  Add the specified element to this queue, 
		 *  waiting if necessary for another thread to receive it.
		 ***********************************************************/
		void put(const T& elem) {
			std::lock_guard<std::mutex> lock(myMutex);
			while(isFull()) {
				notFull.wait(myMutex);
			}
			myQueue.emplace(std::move(elem));
			notEmpty.notify_one;
		}		
		
		/***********************************************************
		 *  Retrieve and remove the head of this queue,
		 *  waiting if necessary for another thread to insert it.
		 *  
		 *  There are two modes for this operation: Blocked or not.
		 *  If "Blocking" is true, then wait if the queue is empty.
		 *  If "Blocking" is false, then give up if the queue is empty.
		 ***********************************************************/
		bool take(T& elem, bool Blocking = true) {
			std::lock_guard<std::mutex> lock(myMutex);
			if (Blocking) {
				// blocking mode
				while (isEmpty()) {
				    notEmpty.wait(myMutex);
				}
			}
			else {
				// non-blocking mode
				if (isEmpty())
					return false;
			}
			elem = myQueue.front();
			myQueue.pop();
			notFull.notify_one;
			return true;
		}
		
		// Get the size of queue
		int getSize() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size();
		}
		
		// Return true is the queue is full.
		bool isFull() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size() == queueMaxSize;
		}
		
		// Return true is the queue is empty.
		bool isEmpty() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return myQueue.size() == 0;
		}
		
		// Get the max size of the queue.
		int getMaxSize() const {
			std::lock_guard<std::mutex> lock(myMutex);
			return queueMaxSize;
		}
		
		// Set the max size of the queue
		void setMaxSize(int maxSize) {
			std::lock_guard<std::mutex> lock(myMutex);
			queueMaxSize = maxSize;
		}

	}
}

#endif
