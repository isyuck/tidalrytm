#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <chrono>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

class Scheduler {
public:
  Scheduler(std::condition_variable &_inAvailable, std::mutex &_inMutex,
            std::condition_variable &_outAvailable, std::mutex &_outMutex)
      : inAvailable(_inAvailable), inMutex(_inMutex),
        outAvailable(_outAvailable), outMutex(_outMutex){};

  // the out queue is for messages that need to be immediately sent
  void
  run(std::queue<std::pair<std::chrono::microseconds,
                           std::vector<std::vector<unsigned char>>>> &inQueue,
      std::queue<std::vector<unsigned char>> &outQueue) {

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
          std::unique_lock<std::mutex> outLock(outMutex);
          bool wasEmpty = outQueue.empty();
          for (const auto &e : el.second) {
            outQueue.push(e);
          }
          outLock.unlock();
          if (wasEmpty) {
            outAvailable.notify_one();
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
  std::condition_variable &outAvailable;
  std::mutex &outMutex;
};

#endif // SCHEDULER_H_
