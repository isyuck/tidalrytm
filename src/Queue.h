#ifndef QUEUE_H_
#define QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

template <class T> class Queue {

public:
  std::queue<T> queue;
  std::mutex mtx;
  std::condition_variable cv;

  void push(const T v) {
    std::unique_lock<std::mutex> lock(this->mtx);
    const bool wasEmpty = this->queue.empty();
    this->queue.push(v);
    if (wasEmpty) {
      cv.notify_one();
    }
  }
  T waitForLatest() {
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
