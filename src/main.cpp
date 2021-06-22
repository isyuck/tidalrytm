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

  Queue<tidalMessage> tidalMessages;
  Queue<midiMessage> midiMessages;

  TidalParser<tidalMessage> tidalParser(OSCPORT, OSCADDR, tidalMessages);

  Scheduler<tidalMessage, midiMessage> scheduler(tidalMessages, midiMessages);

  // immediately sends any messages in the midiMessage queue to the rytm
  MidiOut<midiMessage> midiOut(MIDIPORT, midiMessages);

  tidalParser.run();
  scheduler.run();
  midiOut.run();

  tidalParser.join();
  scheduler.join();
  midiOut.join();
}
