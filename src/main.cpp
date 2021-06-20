#include "MessageQueue.h"
#include "MidiOut.h"
#include "TidalParser.h"
#include "Track.h"
#include "oscpack/osc/OscReceivedElements.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

int main(void) {

  // the rytm's 12 tracks
  // std::array<Track, 12> tracks = {Track(0), Track(1), Track(2),  Track(3),
  //                                 Track(4), Track(5), Track(6),  Track(7),
  //                                 Track(8), Track(9), Track(10), Track(11)};

  // MidiOut rytm;
  // rytm.midiInit();

  std::cout << "starting...\n";
  std::vector<osc::ReceivedMessage> receivedMessages;
  std::condition_variable statesAvailable;
  std::mutex statesMutex;
  TidalParser parser(57120, "/rytm", statesAvailable, statesMutex);

  auto stateGetter = std::thread([&]() {
    for (;;) {
      std::unique_lock<std::mutex> lock(statesMutex);
      while (parser.empty()) {
        statesAvailable.wait(lock);
      }
      auto state = parser.pop();
      lock.unlock();

      std::cout << state;
    }
  });

  parser.run();
  parser.join();
  stateGetter.join();
}
