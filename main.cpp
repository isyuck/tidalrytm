#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"
#include <oscpack/ip/IpEndpointName.h>
#include <rtmidi/RtMidi.h>

#define OSCPORT 57120
#define OSCADDR "/rytm"
#define MIDIPORT 1

// chars and a timestamp
struct MidiMessage {
  // when to send it
  std::chrono::microseconds timestamp;
  // messages ready to send, e.g. cc and note on
  std::vector<std::vector<unsigned char>> msgs;

  bool operator<(MidiMessage const &o) { return timestamp < o.timestamp; }
};

// midi out object. handles connecting to devices and sending messsages
RtMidiOut *midiInit(const int port = 1) {
  RtMidiOut *midiOut = 0;
  try {
    midiOut = new RtMidiOut();
    midiOut->openPort(port);
  } catch (RtMidiError &error) {
    error.printMessage();
    std::exit(EXIT_FAILURE);
  }
  std::string portName = "";
  const auto nPorts = midiOut->getPortCount();
  std::cout << nPorts << " out ports:\n";
  for (unsigned int i = 0; i < nPorts; i++) {
    try {
      portName = midiOut->getPortName(i);
    } catch (RtMidiError &error) {
      error.printMessage();
    }
    std::cout << " #" << i << ": " << portName << '\n';
  }
  std::cout << "\nopening port #" << port << '\n';
  return midiOut;
};

std::mutex oscMutex;
std::condition_variable oscToParse;

std::mutex midiMutex;
std::condition_variable midiToSend;

std::queue<osc::ReceivedMessage> oscMessageQueue;
std::queue<MidiMessage> midiMessageQueue;

class OscReceiver : public osc::OscPacketListener {
protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const osc::IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    // push the msg into the queue of osc messages to be parsed (if its address
    // is the one we want)
    try {
      if (std::strcmp(oscMessage.AddressPattern(), OSCADDR) == 0) {
        std::unique_lock<std::mutex> lock(oscMutex);
        bool wasEmpty = oscMessageQueue.empty();
        oscMessageQueue.push(oscMessage);
        std::cout << "received message!\n";
        lock.unlock();
        // tell waiting threads theres messages to parse
        if (wasEmpty) {
          oscToParse.notify_one();
        }
      }
    } catch (osc::Exception &e) {
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << "\n";
    }
  }
};

// pulls osc messages off the osc queue, parses them into midi messages, and
// pushes them onto the midi to send queue
void oscToMidi(std::queue<osc::ReceivedMessage> &oscQueue,
               std::queue<MidiMessage> &midiQueue) {

  std::unique_lock<std::mutex> oscLock(oscMutex);

  while (oscQueue.empty()) {
    oscToParse.wait(oscLock);
  }
  std::cout << "parsing message!\n";

  // temp
  MidiMessage mm;

  const auto msg = oscQueue.front();
  oscQueue.pop();
  // iterator to the msg's args. the custom boottidal.hs means we know what args
  // will in what positions be ahead of time
  auto args = msg.ArgumentsBegin();

  // tidal sends second and microsends as two seperate ints, this converts them
  // into a std::chrono::microseconds object for ease of use
  const auto sec = std::chrono::seconds((args++)->AsInt32());
  const auto usec = std::chrono::microseconds((args++)->AsInt32());
  const std::chrono::microseconds timestamp = sec + usec;

  mm.timestamp = timestamp;

  // skip over args we dont use yet, e.g. cycle, delta etc
  for (int i = 0; i < 5; i++) {
    args++;
  }

  // midi channel and note from next two args
  const auto chan = (args++)->AsInt32();

  // this channel is 'muted'
  if (chan < 0) {
    return;
  }

  // for some reason tidal sends this as a float
  const auto note = (int)(args++)->AsFloat();

  // iterate over the control change values
  for (int i = 0; i < 127; i++) {
    const auto v = (args++)->AsInt32();
    if (v <= 0 && v <= 127) {
      // put them into the mm's msgs vector
      std::vector<unsigned char> cc{0xb0 | chan, i + 1, v};
      mm.msgs.push_back(cc);
    }
  }

  // cc change setting note value
  std::vector<unsigned char> mmsg = {0xb0 | chan, 3, (note + 60)};
  mm.msgs.push_back(mmsg);

  // note on, channel, velocity
  mmsg = {0x90, chan, 127};
  mm.msgs.push_back(mmsg);

  bool wasEmpty = midiQueue.empty();
  std::unique_lock<std::mutex> midiLock(midiMutex);

  std::cout << "parsed message!\n";
  midiQueue.push(mm);
  midiMutex.unlock();
  if (wasEmpty) {
    midiToSend.notify_one();
  }
}

void sendMidi(RtMidiOut *midiOut, std::queue<MidiMessage> &queue) {

  std::unique_lock<std::mutex> lock(midiMutex);
  // wait for midi messages to send
  while (queue.empty()) {
    midiToSend.wait(lock);
  }
  auto midiMessage = queue.front();

  // get time now
  const auto timeNow = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  // only send messages with a timestamp either now, or in the past
  if (timeNow >= midiMessage.timestamp) {
    queue.pop();
    for (const auto &msg : midiMessage.msgs) {
      midiOut->sendMessage(&msg);
    }
    std::cout << "sent message!\n";
  }
  std::cout << "message too early!\n";
}

int main(void) {

  auto midiOut = midiInit(MIDIPORT);

  // setup osc receiver
  OscReceiver tidal;
  osc::UdpListeningReceiveSocket s(
      osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, OSCPORT), &tidal);

  // start listening
  std::thread receiverThread([&s]() { s.Run(); });

  // spawn some osc parser threads
  auto parser = [&]() {
    for (;;) {
      oscToMidi(oscMessageQueue, midiMessageQueue);
    }
  };
  // std::vector<std::thread> parsers;
  // for (int i = 0; i < 1; i++) {
  //   parsers.push_back(std::thread(parser));
  // }

  std::thread parserThread(parser);

  // spawn one midi sender thread
  std::thread senderThread([&]() {
    for (;;) {
      sendMidi(midiOut, midiMessageQueue);
    }
  });

  // tidy
  receiverThread.join();
  // std::for_each(parsers.begin(), parsers.end(),
  //               [](std::thread &p) { p.join(); });
  senderThread.join();

  return 0;
}
