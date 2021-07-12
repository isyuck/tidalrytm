#ifndef QUEUECONSUMER_H_
#define QUEUECONSUMER_H_

#include "Queue.h"

#include <algorithm>
#include <thread>

// IT = in type of queue, OT = out type
template <class IT, class OT> class QueueConsumer {
public:
  explicit QueueConsumer(Queue<IT> &in, Queue<OT> &out) : in(in), out(out) {}

  //  start n threads, running main() 'forever'
  void run(const int n = 1) {
    for (int i = 0; i < n; i++) {
      this->threads.push_back(std::thread([&]() {
        for (;;) {
          this->main();
        }
      }));
    }
  }

  // join all threads
  void join() {
    std::for_each(this->threads.begin(), this->threads.end(),
                  [](std::thread &t) { t.join(); });
  }

protected:
  virtual void main() {}

  // pause the thread until a new IT is in the in queue
  constexpr IT wait() {
    const IT e = this->in.wait();
    return e;
  }

  // pop from the in queue
  constexpr void pop() { this->in.pop(); }

  // push to the out queue
  constexpr void push(const OT &e) { this->out.push(e); }

  Queue<IT> &in;
  Queue<OT> &out;
  std::vector<std::thread> threads;
};

#endif // QUEUECONSUMER_H_
