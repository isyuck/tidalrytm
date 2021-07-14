#ifndef QUEUE_H_
#define QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

template <class T = int> class Queue {

public:
  Queue<T>() {}
  Queue<T>(const std::queue<T> &q) { this->queue = q; }

  Queue<T> operator=(const Queue<T> &q) {
    std::unique_lock<std::mutex> lock(this->mtx);
    return this->queue;
  }

  bool operator==(const Queue<T> &q) {
    std::unique_lock<std::mutex> lock(this->mtx);
    return this->queue == q.queue;
  }

  bool operator!=(const Queue<T> &q) {
    std::unique_lock<std::mutex> lock(this->mtx);
    return this->queue != q.queue;
  }

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

private:
  std::queue<T> queue;
  std::mutex mtx;
  std::condition_variable cv;
};

#endif // QUEUE_H_
