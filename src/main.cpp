#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "Scheduler.h"
#include "StateHandler.h"
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

  std::queue<State> statesToParse;
  std::mutex statesToParseMutex;
  std::condition_variable statesToParseAvailable;

  std::queue<std::vector<unsigned char>> midiMessages;
  std::mutex midiMessageMutex;
  std::condition_variable midiMessagesAvailable;

  std::queue<std::pair<std::chrono::microseconds,
                       std::vector<std::vector<unsigned char>>>>
      messagesToSched;
  std::mutex messagesToSchedMutex;
  std::condition_variable messagesToSchedAvailable;

  // listen and convert osc from tidal to 'states', a channel, usec timestamp,
  // and list of cc values
  TidalParser tidalParser(OSCPORT, OSCADDR, statesToParseAvailable,
                          statesToParseMutex);

  StateHandler stateHandler(statesToParseAvailable, statesToParseMutex,
                            messagesToSchedAvailable, messagesToSchedMutex);

  Scheduler scheduler(messagesToSchedAvailable, messagesToSchedMutex,
                      midiMessagesAvailable, midiMessageMutex);

  MidiOut midiOut(MIDIPORT, midiMessagesAvailable, midiMessageMutex);

  tidalParser.run(statesToParse);
  stateHandler.run(statesToParse, messagesToSched);
  scheduler.run(messagesToSched, midiMessages);
  midiOut.run(midiMessages);

  tidalParser.join();
  stateHandler.join();
  scheduler.join();
  midiOut.join();
}
