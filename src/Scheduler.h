#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "Queue.h"

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
  void run() {
    this->mainThread = std::thread([&]() {
      for (;;) {
        // pause this thread here unless there is a new message available
        const auto message = in.wait();

        // TODO maybe loop with the current element here? and break when sent

        // get the time now in microseconds
        const auto timeNow =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch());

        // check timestamp (first in pair) of the message, if it's in the past
        // send the message to the midi queue
        if (timeNow >= message.first) {
          in.pop();
          for (const auto &e : message.second) {
            out.push(e);
          }
        }
      }
    });
  }
  void join() { this->mainThread.join(); }

private:
  std::thread mainThread;
  Queue<IT> &in;
  Queue<OT> &out;
};

#endif // SCHEDULER_H_
