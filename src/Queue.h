#ifndef QUEUE_H_
#define QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

template <class T = int> class Queue {

private:
  std::queue<T> queue;
  std::mutex mtx;
  std::condition_variable cv;

public:
  // push and notify waiting all threads
  void push(const T &v) {
    std::unique_lock<std::mutex> lock(this->mtx);
    const bool wasEmpty = this->queue.empty();
    this->queue.push(v);
    if (wasEmpty) {
      cv.notify_one();
    }
  }
  // pause the current thread until notified of a new push
  T wait() {
    std::unique_lock<std::mutex> lock(this->mtx);
    while (this->queue.empty()) {
      cv.wait(lock);
    }
    return this->queue.front();
  }

  void pop() {
    std::unique_lock<std::mutex> lock(this->mtx);
    this->queue.pop();
  }
};

#endif // QUEUE_H_
