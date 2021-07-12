#ifndef MIDIOUT_H_
#define MIDIOUT_H_

#include <rtmidi/RtMidi.h>

#include "Queue.h"
#include "QueueConsumer.h"

template <class IT> class MidiOut : public QueueConsumer<IT, int> {
public:
  using QueueConsumer<IT, int>::QueueConsumer;

  void openPort(const int port) {
    try {
      midiOut = new RtMidiOut();
      midiOut->openPort(port);
    } catch (RtMidiError &error) {
      error.printMessage();
      std::exit(EXIT_FAILURE);
    }
    std::cout << "opened port #" << port << " (" << midiOut->getPortName(port)
              << ")\n";
  }

  // send messages to the rytm
  void main() {
    const auto msg = this->wait();
    this->pop();
    this->midiOut->sendMessage(&msg);
  }

private:
  RtMidiOut *midiOut;
};

#endif // MIDIOUT
