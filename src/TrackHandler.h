#ifndef TRACKHANDLER_H_
#define TRACKHANDLER_H_

#include "State.h"
#include "Track.h"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

// take all of the state changes, update the state of each track,
// notify a condvar of track state changes
class TrackHandler {
public:
  TrackHandler(std::condition_variable &_statesAvailable,
               std::mutex &_statesMutex)
      : statesMutex(_statesMutex), statesAvailable(_statesAvailable) {}

  void run(std::queue<State> &stateQueue) {
    this->updateThread = std::thread([&]() { this->update(stateQueue); });
  }

  void join() { this->updateThread.join(); }

private:
  std::thread updateThread;
  std::mutex &statesMutex;
  std::condition_variable &statesAvailable;

  // update the appropriate tracks
  void update(std::queue<State> &stateQueue) {
    for (;;) {

      std::unique_lock<std::mutex> lock(statesMutex);
      while (stateQueue.empty()) {
        // std::cout << "waiting for states...\n";
        statesAvailable.wait(lock);
      }
      // std::cout << "doing something with a state!\n";
      auto newState = stateQueue.front();
      stateQueue.pop();
      lock.unlock();

      auto trackState = tracks[newState.channel].getStateRef();
      if (trackState != newState.cc) {
        for (int i = 0; i < 127; i++) {
          if (trackState[i] != newState.cc[i]) {
            // trackState[i] = newState.cc[i];
          }
        }
      }

      std::cout << newState;
    }
  }

  // the rytm's 12 tracks
  std::array<Track, 12> tracks = {Track(0), Track(1), Track(2),  Track(3),
                                  Track(4), Track(5), Track(6),  Track(7),
                                  Track(8), Track(9), Track(10), Track(11)};
};

#endif // TRACKHANDLER_H_
