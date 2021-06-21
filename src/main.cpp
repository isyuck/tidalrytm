#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "Scheduler.h"
#include "TidalParser.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#define MIDIPORT 1
#define OSCPORT 57120
#define OSCADDR "/rytm"

int main(void) {

  std::cout << "starting...\n";

  // messages for the scheduler
  std::queue<std::pair<std::chrono::microseconds,
                       std::vector<std::vector<unsigned char>>>>
      messagesToSched;
  std::mutex messagesToSchedMutex;
  std::condition_variable messagesToSchedAvailable;

  // messages to be sent immediately
  std::queue<std::vector<unsigned char>> midiMessages;
  std::mutex midiMessageMutex;
  std::condition_variable midiMessagesAvailable;

  // listen and convert osc from tidal to messages that store a timestamp and
  // midi control change info, that the scheduler can use
  TidalParser tidalParser(OSCPORT, OSCADDR, messagesToSchedAvailable,
                          messagesToSchedMutex);

  // pull from the queue of parsed messages, and send them to the queue that
  // midi out pulls from
  Scheduler scheduler(messagesToSchedAvailable, messagesToSchedMutex,
                      midiMessagesAvailable, midiMessageMutex);

  // immediately sends any messages in the midiMessage queue to the rytm
  MidiOut midiOut(MIDIPORT, midiMessagesAvailable, midiMessageMutex);

  tidalParser.run(messagesToSched);
  scheduler.run(messagesToSched, midiMessages);
  midiOut.run(midiMessages);

  tidalParser.join();
  scheduler.join();
  midiOut.join();
}
