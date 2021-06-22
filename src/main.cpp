#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "Queue.h"
#include "Scheduler.h"
#include "TidalParser.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#define MIDIPORT 1
#define OSCPORT 57120
#define OSCADDR "/rytm"

using tidalMessage = std::pair<std::chrono::microseconds,
                               std::vector<std::vector<unsigned char>>>;

using midiMessage = std::vector<unsigned char>;

int main(void) {

  // messages for the scheduler
  std::queue<std::pair<std::chrono::microseconds,
                       std::vector<std::vector<unsigned char>>>>
      messagesToSched;
  std::mutex messagesToSchedMutex;
  std::condition_variable messagesToSchedAvailable;

  Queue<midiMessage> midiMessages;

  TidalParser tidalParser(OSCPORT, OSCADDR, messagesToSchedAvailable,
                          messagesToSchedMutex);

  Scheduler<midiMessage> scheduler(messagesToSchedAvailable,
                                   messagesToSchedMutex, midiMessages);

  // immediately sends any messages in the midiMessage queue to the rytm
  MidiOut<midiMessage> midiOut(MIDIPORT, midiMessages);

  tidalParser.run(messagesToSched);
  scheduler.run(messagesToSched);
  midiOut.run();

  tidalParser.join();
  scheduler.join();
  midiOut.join();
}
