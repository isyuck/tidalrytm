#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "TidalParser.h"
#include "TrackHandler.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

int main(void) {

  // MidiOut rytm;
  // rytm.midiInit();

  std::cout << "starting...\n";
  std::condition_variable statesAvailable;
  std::mutex statesMutex;

  // stores states that haven't yet been used by the track handler
  std::queue<State> stateQueue;

  TidalParser tidalParser(57120, "/rytm", statesAvailable, statesMutex);
  TrackHandler trackHandler(statesAvailable, statesMutex);

  tidalParser.run(stateQueue);
  trackHandler.run(stateQueue);
  tidalParser.join();
  trackHandler.join();
}
