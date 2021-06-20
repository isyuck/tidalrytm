#include <rtmidi/RtMidi.h>
#include <vector>

class MidiOut {
public:
  void midiInit(const int port = 1) {
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

  void
  sendMessage(const std::vector<std::vector<unsigned char>> &messages) const {
    for (auto &m : messages) {
      this->midiOut->sendMessage(&m);
    }
  }

private:
  RtMidiOut *midiOut;
};
