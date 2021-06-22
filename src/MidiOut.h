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
  MidiOut(const int port, Queue<IT> &in) : in(in) {

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
    std::cout << "\nopened port #" << port << '\n';
  };

  // send messages to the rytm
  void run() {
    this->thread = std::thread([&]() {
      for (;;) {
        const auto msg = in.wait();
        in.pop();
        this->midiOut->sendMessage(&msg);
      }
    });
  }
  void join() { this->thread.join(); }

private:
  Queue<IT> &in;
  std::thread thread;
  RtMidiOut *midiOut;
};

#endif // MIDIOUT
