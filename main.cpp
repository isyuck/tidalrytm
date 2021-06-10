#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <mutex>
#include <ratio>
#include <rtmidi/RtMidi.h>
#include <signal.h>
#include <thread>

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"
#include <oscpack/ip/IpEndpointName.h>

#define PORT 57120

std::mutex mtx;
std::vector<osc::ReceivedMessage> oscMessages;

RtMidiOut *midiInit() {
  RtMidiOut *midiOut = 0;
  try {
    midiOut = new RtMidiOut();
    midiOut->openPort(1);
  } catch (RtMidiError &error) {
    error.printMessage();
    std::exit(EXIT_FAILURE);
  }
  std::string portName = "";
  const auto nPorts = midiOut->getPortCount();
  std::cout << "\nThere are " << nPorts << " MIDI output ports available.\n";
  for (unsigned int i = 0; i < nPorts; i++) {
    try {
      portName = midiOut->getPortName(i);
    } catch (RtMidiError &error) {
      error.printMessage();
    }
    std::cout << "  Output Port #" << i << ": " << portName << '\n';
  }
  std::cout << '\n';
  return midiOut;
};

struct MidiMessage {
  // when to send it
  std::chrono::microseconds timestamp;
  // messages ready to send, e.g. cc and note on
  std::vector<std::vector<unsigned char>> msgs;
};

class OscReceiver : public osc::OscPacketListener {
protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const osc::IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    try {
      if (std::strcmp(oscMessage.AddressPattern(), "/rytm") == 0) {
        mtx.lock();
        oscMessages.push_back(oscMessage);
        mtx.unlock();
      }
    } catch (osc::Exception &e) {
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << "\n";
    }
  }
};

std::vector<MidiMessage>
parseOscMessages(std::vector<osc::ReceivedMessage> messages) {

  const auto timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  std::vector<MidiMessage> midiMessages;

  auto iter = [&messages, &midiMessages]() {
    for (auto &message : messages) {
      auto args = message.ArgumentsBegin();

      MidiMessage mm;

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
      midiMessages.push_back(mm);
    }
    return midiMessages;
  };
  return iter();
}

void sendMidi(RtMidiOut *midiOut, const std::vector<MidiMessage> &messages) {

  for (const auto &message : messages) {

    // get time now
    const auto timeNow = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    if (message.timestamp <= timeNow) {
      for (const auto &msg : message.msgs) {
        midiOut->sendMessage(&msg);
      }
    }
  }
}

int main(void) {

  // exit on sigint
  signal(SIGINT, [](int sn) { std::exit(EXIT_SUCCESS); });

  // open midi port
  auto midiOut = midiInit();

  // setup osc receiver
  OscReceiver tidal;
  osc::UdpListeningReceiveSocket s(
      osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, PORT), &tidal);
  // start listening
  std::thread receiverThread([&s]() { s.Run(); });

  for (;;) {
    mtx.lock();
    auto messages = oscMessages;
    oscMessages.clear();
    mtx.unlock();

    auto parsed = parseOscMessages(messages);
    sendMidi(midiOut, parsed);
  }
  return 0;
}
