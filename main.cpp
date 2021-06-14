#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <ratio>
#include <signal.h>
#include <thread>

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"
#include <oscpack/ip/IpEndpointName.h>
#include <rtmidi/RtMidi.h>

#define OSCPORT 57120
#define OSCADDR "/rytm"
#define MIDIPORT 1

struct MidiMessage {
  // when to send it
  std::chrono::microseconds timestamp;
  // messages ready to send, e.g. cc and note on
  std::vector<std::vector<unsigned char>> msgs;
};

std::mutex oscMutex;
std::mutex midiMutex;
std::queue<osc::ReceivedMessage> oscMessages;
std::queue<MidiMessage> midiMessages;

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

class OscReceiver : public osc::OscPacketListener {
protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const osc::IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    try {
      // put the msg in oscMessages (if its address is the one we want)
      if (std::strcmp(oscMessage.AddressPattern(), OSCADDR) == 0) {
        oscMutex.lock();
        oscMessages.push(oscMessage);
        std::cout << "received osc message\n";
        std::cout << "size of osc message queue:" << oscMessages.size() << '\n';
        oscMutex.unlock();
      }
    } catch (osc::Exception &e) {
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << "\n";
    }
  }
};

void parseOscMessages(std::queue<osc::ReceivedMessage> &oscMessages,
                      std::queue<MidiMessage> &midiMessages) {

  while (!oscMessages.empty()) {

    midiMutex.lock();
    auto oscMessage = oscMessages.front();
    midiMutex.unlock();
    auto args = oscMessage.ArgumentsBegin();

    MidiMessage mm;
    std::cout << "parsing osc message\n";

    // construct timestamp from first two args
    const auto sec = std::chrono::seconds((args++)->AsInt32());
    const auto usec = std::chrono::microseconds((args++)->AsInt32());
    const std::chrono::microseconds timestamp = sec + usec;

    // skip over args we dont use yet
    for (int i = 0; i < 5; i++) {
      args++;
    }

    // midi channel and note from next two args
    const auto chan = (args++)->AsInt32();
    if (chan < 0) {
      break;
    }
    const auto note = (int)(args++)->AsFloat();

    // iterate over the control change values
    for (int i = 0; i < 127; i++) {
      const auto v = (args++)->AsInt32();
      // ignore some values (read: do not change value)
      if (v >= 0) {
        // put them into the mm's msgs vector
        std::vector<unsigned char> cc{0xb0 | chan, i + 1, v};
        mm.msgs.push_back(cc);
      }
    }
    // cc change setting note value
    std::vector<unsigned char> msg = {0xb0 | chan, 3, (note + 60)};
    mm.msgs.push_back(msg);
    // note on, channel, velocity
    msg = {0x90, chan, 127};
    mm.msgs.push_back(msg);

    midiMutex.lock();
    midiMessages.push(mm);
    midiMutex.unlock();

    oscMutex.lock();
    oscMessages.pop();
    oscMutex.unlock();
  }
}

void sendMidi(RtMidiOut *midiOut, std::queue<MidiMessage> &midiMessages) {

  while (!midiMessages.empty()) {
    midiMutex.lock();
    const auto midiMessage = midiMessages.front();
    midiMutex.unlock();

    // get time now
    const auto timeNow = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    // if we have passed the messages timestamp, send it
    if (midiMessage.timestamp <= timeNow) {
      for (const auto &msg : midiMessage.msgs) {
        midiOut->sendMessage(&msg);
      }
    }
    midiMutex.lock();
    midiMessages.pop();
    midiMutex.unlock();
  }
}

int main(void) {

  // exit on sigint
  signal(SIGINT, [](int sn) { std::exit(EXIT_SUCCESS); });

  // open midi port
  auto midiOut = midiInit(MIDIPORT);

  // setup osc receiver
  OscReceiver tidal;
  osc::UdpListeningReceiveSocket s(
      osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, OSCPORT), &tidal);
  // start listening
  std::thread receiverThread([&s]() { s.Run(); });

  std::thread senderThread([&]() {
    for (;;) {
      sendMidi(midiOut, midiMessages);
    }
  });

  std::thread parserThread([&]() {
    for (;;) {
      parseOscMessages(oscMessages, midiMessages);
    }
  });

  return 0;
}
