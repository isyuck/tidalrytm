#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "Queue.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

template <class IT, class OT> class Scheduler {
public:
  Scheduler(Queue<IT> &in, Queue<OT> &out) : in(in), out(out) {}

  // the out queue is for messages that need to be immediately sent
  // TODO multithread this
  void run(const int nThreads = 1) {
    for (int i = 0; i < nThreads; i++) {
      this->threads.push_back(std::thread([&]() { this->schedule(); }));
    }
  }
  void join() {
    std::for_each(this->threads.begin(), this->threads.end(),
                  [](std::thread &p) { p.join(); });
  }

private:
  void schedule() {
    for (;;) {
      // pause this thread here unless there is a new message available
      const auto msg = in.wait();

      // keep checking whether the current message we have needs to be sent
      for (;;) {

        // get the time now in microseconds
        const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch());

        // check timestamp (first in pair) of the message, if it's in the past
        // send the message to the midi queue
        if (now >= msg.first) {
          in.pop();
          for (const auto &cc : msg.second) {
            out.push(cc);
          }
          break;
        }
      }
    }
  }

  std::vector<std::thread> threads;
  Queue<IT> &in;
  Queue<OT> &out;
};

#endif // SCHEDULER_H_
