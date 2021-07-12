#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "Queue.h"
#include "Scheduler.h"
#include "SendOsc.h"
#include "TidalParser.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#define MIDIPORT 1
#define INPORT 57120
#define INADDR "/rytm"
#define OUTPORT 57121
#define OUTADDR "/rytmvis"

using tidalMessage = std::pair<std::chrono::microseconds,
                               std::vector<std::vector<unsigned char>>>;

using midiMessage = std::vector<unsigned char>;

int main(int argc, char **argv) {

  // TODO use args for optional vis osc

  Queue<tidalMessage> tidalMessages;
  Queue<midiMessage> midiMessages;

  // listen and parse osc from tidal into a timestamped midi message
  TidalParser<tidalMessage> tidalParser(INPORT, INADDR, tidalMessages);

  // pulls from the midi messages and send to midiout when appropriate
  Scheduler<tidalMessage, midiMessage> scheduler(tidalMessages, midiMessages);

  // immediately sends any messages in the midiMessage queue to the rytm
  MidiOut<midiMessage> midiOut(MIDIPORT, midiMessages);

  // arg is n threads
  tidalParser.run(8);
  scheduler.run(8);
  midiOut.run();

  tidalParser.join();
  scheduler.join();
  midiOut.join();
}
