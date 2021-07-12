#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "Queue.h"
#include "QueueConsumer.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

template <class IT, class OT> class Scheduler : public QueueConsumer<IT, OT> {
public:
  using QueueConsumer<IT, OT>::QueueConsumer;

protected:
  void main() {
    // pause this thread here unless there is a new message available
    const auto msg = this->wait();

    // keep checking whether the current message we have needs to be sent
    for (;;) {

      // get the time now in microseconds
      const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch());

      // check timestamp (first in pair) of the message, if it's in the past
      // send the message to the midi queue
      if (now >= msg.first) {
        this->pop();
        for (const auto &cc : msg.second) {
          this->push(cc);
        }
        break;
      }
    }
  }
};

#endif // SCHEDULER_H_
