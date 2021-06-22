#ifndef MIDIOUT_H_
#define MIDIOUT_H_

#include <rtmidi/RtMidi.h>

#include "Queue.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

template <class IT> class MidiOut {
public:
  MidiOut(const int port, Queue<IT> &_inQueue) : inQueue(_inQueue) {

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
  };

  void run() {
    this->mainThread = std::thread([&]() {
      for (;;) {
        this->sendMessages();
      }
    });
  }
  void join() { this->mainThread.join(); }

private:
  Queue<IT> &inQueue;
  std::thread mainThread;
  RtMidiOut *midiOut;

  void sendMessages() const {
    const auto message = this->inQueue.waitForLatest();
    this->inQueue.pop();

    // std::cout << "midi out! ccn: " << +message[1] << ", ccv: " << +message[2]
    //           << '\n';
    this->midiOut->sendMessage(&message);
  }
};

#endif // MIDIOUT
