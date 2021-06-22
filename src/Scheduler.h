#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "Queue.h"

#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

template <class OT> class Scheduler {
public:
  Scheduler(std::condition_variable &_inAvailable, std::mutex &_inMutex,
            Queue<OT> &_outQueue)
      : inAvailable(_inAvailable), inMutex(_inMutex), outQueue(_outQueue) {}

  // the out queue is for messages that need to be immediately sent
  void
  run(std::queue<std::pair<std::chrono::microseconds,
                           std::vector<std::vector<unsigned char>>>> &inQueue) {

    this->mainThread = std::thread([&]() {
      for (;;) {
        std::unique_lock<std::mutex> inLock(inMutex);
        while (inQueue.empty()) {
          // std::cout << "waiting for states...\n";
          inAvailable.wait(inLock);
        }
        // std::cout << "doing something with a state!\n";
        const auto el = inQueue.front();
        inLock.unlock();

        const auto timeNow =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch());

        if (timeNow >= el.first) {
          inLock.lock();
          inQueue.pop();
          for (const auto &e : el.second) {
            this->outQueue.push(e);
          }
          inLock.unlock();
        }
      }
    });
  }
  void join() { this->mainThread.join(); }

private:
  std::thread mainThread;
  std::condition_variable &inAvailable;
  std::mutex &inMutex;
  Queue<OT> &outQueue;
};

#endif // SCHEDULER_H_
