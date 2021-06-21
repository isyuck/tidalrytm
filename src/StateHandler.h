#ifndef STATEHANDLER_H_
#define STATEHANDLER_H_

#include "State.h"

#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

// take all of the state changes, update the state of each track,
// notify a condvar of track state changes
class StateHandler {
public:
  StateHandler(std::condition_variable &_inAvailable, std::mutex &_inMutex,
               std::condition_variable &_outAvailable, std::mutex &_outMutex)
      : inAvailable(_inAvailable), inMutex(_inMutex),
        outAvailable(_outAvailable), outMutex(_outMutex) {}

  // run the threads that update the tracks
  // TODO mutex & condvar for the track array!!!
  // TODO do it soon!
  // TODO TODO TODO don't forget!!!!!
  void run(std::queue<State> &statesToParse,
           std::queue<std::pair<std::chrono::microseconds,
                                std::vector<std::vector<unsigned char>>>>
               &messagesToSched) {
    for (int i = 0; i < 8; i++) {
      this->updateThreads.push_back(
          std::thread([&]() { this->update(statesToParse, messagesToSched); }));
    }
  }

  void join() {
    std::for_each(this->updateThreads.begin(), this->updateThreads.end(),
                  [](std::thread &p) { p.join(); });
  }

private:
  // std::mutex &statesToParseMutex;
  // std::condition_variable &statesToParseAvailable;
  // std::mutex &midiMessageMutex;
  // std::condition_variable &midiMessagesAvailable;
  std::vector<std::thread> updateThreads;

  std::condition_variable &inAvailable;
  std::mutex &inMutex;
  std::condition_variable &outAvailable;
  std::mutex &outMutex;

  std::mutex tracksMutex;

  // update the appropriate tracks
  void update(std::queue<State> &statesToParse,
              std::queue<std::pair<std::chrono::microseconds,
                                   std::vector<std::vector<unsigned char>>>>
                  &messagesToSched) {
    for (;;) {

      std::unique_lock<std::mutex> inLock(inMutex);
      while (statesToParse.empty()) {
        // std::cout << "waiting for states...\n";
        inAvailable.wait(inLock);
      }
      // std::cout << "doing something with a state!\n";
      auto newState = statesToParse.front();
      statesToParse.pop();
      inLock.unlock();

      tracksMutex.lock();
      const std::array<unsigned char, 127> oldStateCC = tracks[0].cc;
      tracksMutex.unlock();
      const std::array<unsigned char, 127> newStateCC = newState.cc;

      // update differing values between the old and new states
      // TODO only doing track[0], channels are currently broken in patterns
      // and get converted to floats, so tr "0 1" gives "0 255" on the cpp
      // end...
      std::vector<std::vector<unsigned char>> changes;
      if (oldStateCC != newStateCC) {
        for (int i = 0; i < 127; i++) {
          if (newStateCC[i] <= 127) {
            if (oldStateCC[i] != newStateCC[i]) {
              tracksMutex.lock();
              tracks[0].cc[i] = newStateCC[i];
              const unsigned char ccn = i + 1;
              const unsigned char ccv = tracks[0].cc[i];
              changes.push_back({0xb0, ccn, ccv});
              tracksMutex.unlock();
              std::cout << i + 1 << " : " << +(tracks[0].cc[i]) << '\n';
            }
          }
        }
        std::cout << "--------------------\n";
      }
      std::cout << "message!\n";
      // TODO change note value / cc

      // note on
      changes.push_back({0x90, 0, 127});

      std::unique_lock<std::mutex> outLock(outMutex);
      bool wasEmpty = messagesToSched.empty();
      messagesToSched.push(std::make_pair(newState.timestamp, changes));
      outLock.unlock();
      if (wasEmpty) {
        outAvailable.notify_one();
      }

      // std::cout << tracks[0];
    }
  }

  // the rytm's 12 tracks
  std::array<State, 12> tracks;
};

#endif // STATEHANDLER_H_
