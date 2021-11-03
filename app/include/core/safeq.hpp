
/**
 * @file 	safeq.hpp
 * @brief   thread-safe queue
 */

#ifndef _MOLDLINK_SAFE_QUEUE_HPP_
#define _MOLDLINK_SAFE_QUEUE_HPP_

#include <mutex>
#include <condition_variable>

#include <queue>
#include <utility>

template<class T>
class safeQueue {

	std::queue<T> q;

	std::mutex mtx;
	std::condition_variable cv;

	std::condition_variable sync_wait;
	bool finish_processing = false;
	int sync_counter = 0;

	void DecreaseSyncCounter() {
		if (--sync_counter == 0) {
			sync_wait.notify_one();
		}
	}

public:

	typedef typename std::queue<T>::size_type size_type;

	safeQueue() {}

	~safeQueue() {
		finish();
	}

	void produce(T&& item) {
		std::lock_guard<std::mutex> lock(mtx);

		q.push(std::move(item));
		cv.notify_one();
	}

	size_type size() {
		std::lock_guard<std::mutex> lock(mtx);
		return q.size();
	}

	bool consume(T& item) {
		std::lock_guard<std::mutex> lock(mtx);

		if (q.empty()) {
			return false;
		}
		item = std::move(q.front());
		q.pop();

		return true;
	}

	bool consumeSync(T& item) {

		std::unique_lock<std::mutex> lock(mtx);

		sync_counter++;

		cv.wait(lock, [&] {
			return !q.empty() || finish_processing;
		});

		if (q.empty()) {
			DecreaseSyncCounter();
			return false;
		}

		item = std::move(q.front());
		q.pop();

		DecreaseSyncCounter();
		return true;

	}

	void finish() {

		std::unique_lock<std::mutex> lock(mtx);

		finish_processing = true;
		cv.notify_all();

		sync_wait.wait(lock, [&]() {
			return sync_counter == 0;
		});

		finish_processing = false;

	}

};

// #include <queue>
// #include <mutex>

// using namespace std;

// template<typename T>
// class safeQueue {
//     std::queue<T> queue_;
//     mutable std::mutex mutex_;

//     // Moved out of public interface to prevent races between this
//     // and pop().
//     bool empty() const {
//         return queue_.empty();
//     }

//     public:
//         safeQueue() = default;
//         safeQueue(const safeQueue<T> &) = delete ;
//         safeQueue& operator=(const safeQueue<T> &) = delete ;

//         safeQueue(safeQueue<T>&& other) {
//             std::lock_guard<std::mutex> lock(mutex_);
//             queue_ = std::move(other.queue_);
//         }

//         virtual ~safeQueue() { }
        
//         unsigned long size() const {
//             std::lock_guard<std::mutex> lock(mutex_);
//             return queue_.size();
//         }

//         std::optional<T> pop() {
//             std::lock_guard<std::mutex> lock(mutex_);
//             if (queue_.empty()) {
//                 return {};
//             }
//             T tmp = queue_.front();
//             queue_.pop();
//             return tmp;
//         }

//         void push(const T &item) {
//             std::lock_guard<std::mutex> lock(mutex_);
//             queue_.push(item);
//         }
// };

#endif