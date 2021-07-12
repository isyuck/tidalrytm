#ifndef QUEUECONSUMER_H_
#define QUEUECONSUMER_H_

#include "Queue.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <thread>

// IT = in type of queue, OT = out type
template <class IT = int, class OT = int> class QueueConsumer {
public:
  // these constructors handle queue optionality
  // in only
  explicit QueueConsumer(std::reference_wrapper<Queue<IT>> in)
      : in(std::make_optional<std::reference_wrapper<Queue<IT>>>(in)),
        out(std::nullopt) {}

  // out only
  explicit QueueConsumer(std::reference_wrapper<Queue<OT>> out)
      : in(std::nullopt),
        out(std::make_optional<std::reference_wrapper<Queue<OT>>>(out)) {}

  // in and out
  explicit QueueConsumer(std::reference_wrapper<Queue<IT>> in,
                         std::reference_wrapper<Queue<OT>> out)
      : in(std::make_optional<std::reference_wrapper<Queue<IT>>>(in)),
        out(std::make_optional<std::reference_wrapper<Queue<OT>>>(out)) {}

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
    const IT e = this->in.value().get().wait();
    return e;
  }

  // pop from the in queue
  constexpr void pop() { this->in.value().get().pop(); }

  // push to the out queue
  constexpr void push(const OT &e) { this->out.value().get().push(e); }

  // both queues are optional
  std::optional<std::reference_wrapper<Queue<IT>>> in;
  std::optional<std::reference_wrapper<Queue<OT>>> out;

  std::vector<std::thread> threads;
};

#endif // QUEUECONSUMER_H_
