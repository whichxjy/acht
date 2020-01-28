#ifndef _SYNC_QUEUE_HPP_
#define _SYNC_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace acht {

    template <typename T>
    class SyncQueue {
    private:
        std::queue<T> my_queue;
        int queue_max_size;
        mutable std::mutex my_mutex;
        std::condition_variable not_empty;
        std::condition_variable not_full;
        bool need_to_stop;

        /***********************************************************
         *  A helper function that adds element to the queue,
         *  waiting if queue is full.
         ***********************************************************/
        template <typename Type>
        void putHelper(Type&& elem) {
            std::unique_lock<std::mutex> lock(my_mutex);
            while (!need_to_stop && full()) {
                not_full.wait(lock);
            }
            if (need_to_stop) {
                return;
            }
            my_queue.emplace(std::forward<Type>(elem));
            not_empty.notify_one();
        }

        /***********************************************************
         *  Check if the queue is full without lock.
         ***********************************************************/
        bool full() const {
            return my_queue.size() == queue_max_size;
        }

        /***********************************************************
         *  Check if the queue is empty without lock.
         ***********************************************************/
        bool empty() const {
            return my_queue.size() == 0;
        }

    public:
        SyncQueue(int maxSize) : queue_max_size(maxSize), need_to_stop(false) {}

        ~SyncQueue() {
            // If the queue wasn't stoped, then stop it.
            stop();
        }

        // No copy
        SyncQueue(const SyncQueue&) = delete;

        // No assignment
        SyncQueue& operator=(const SyncQueue&) = delete;

        /***********************************************************
         *  Add the given element to this queue,
         *  waiting if queue is full.
         ***********************************************************/
        void put(const T& elem) {
            putHelper(elem);
        }

        void put(T&& elem) {
            putHelper(std::forward<T>(elem));
        }

        /***********************************************************
         *  Retrieve and remove the head of this queue.
         *
         *  There are two modes for this operation: Blocked or not.
         *  If "blocking" is true, then wait if the queue is empty.
         *  If "blocking" is false, then give up if the queue is empty.
         ***********************************************************/
        bool take(T& elem, bool blocking = true) {
            std::unique_lock<std::mutex> lock(my_mutex);
            if (blocking) {
                // blocking mode
                while (!need_to_stop && empty()) {
                    not_empty.wait(lock);
                }
            }
            else {
                // non-blocking mode
                if (empty()) {
                    return false;
                }
            }
            if (need_to_stop) {
                return false;
            }
            // Take element
            elem = my_queue.front();
            my_queue.pop();
            not_full.notify_one();
            return true;
        }

        /***********************************************************
         *  Retrieve and remove all elements of this queue.
         *
         *  There are two modes for this operation: Blocked or not.
         *  If "blocking" is true, then wait if the queue is empty.
         *  If "blocking" is false, then give up if the queue is empty.
         ***********************************************************/
        bool takeAll(std::queue<T> &other_queue, bool blocking = true) {
            std::unique_lock<std::mutex> lock(my_mutex);
            if (blocking) {
                // blocking mode
                while (!need_to_stop && empty()) {
                    not_empty.wait(lock);
                }
            }
            else {
                // non-blocking mode
                if (empty()) {
                    return false;
                }
            }
            if (need_to_stop) {
                return false;
            }
            // Take all elements
            other_queue = std::move(my_queue);
            not_full.notify_one();
            return true;
        }

        /***********************************************************
         *  Start queue
         ***********************************************************/
        void start() {
            std::lock_guard<std::mutex> lock(my_mutex);
            // If the queue was stoped, then restart it.
            if (need_to_stop) {
                need_to_stop = false;
            }
        }

        /***********************************************************
         *  Stop all operations
         ***********************************************************/
        void stop() {
            std::lock_guard<std::mutex> lock(my_mutex);
            // If the queue wasn't stoped, then stop it.
            if (!need_to_stop) {
                need_to_stop = true;
                // Unblocks all threads waiting currently
                not_full.notify_all();
                not_empty.notify_all();
            }
        }

        // Get the size of queue
        int getSize() const {
            std::lock_guard<std::mutex> lock(my_mutex);
            return my_queue.size();
        }

        // Return true is the queue is full.
        bool isFull() const {
            std::lock_guard<std::mutex> lock(my_mutex);
            return my_queue.size() == queue_max_size;
        }

        // Return true is the queue is empty.
        bool isEmpty() const {
            std::lock_guard<std::mutex> lock(my_mutex);
            return my_queue.size() == 0;
        }

        // Get the max size of the queue.
        int getMaxSize() const {
            std::lock_guard<std::mutex> lock(my_mutex);
            return queue_max_size;
        }

        // Set the max size of the queue
        void setMaxSize(int maxSize) {
            std::lock_guard<std::mutex> lock(my_mutex);
            queue_max_size = maxSize;
        }

        // Clear all the elements.
        void clear() {
            std::lock_guard<std::mutex> lock(my_mutex);
            while (!my_queue.empty()) {
                my_queue.pop();
            }
        }

    };
}

#endif