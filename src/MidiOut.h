#ifndef MIDIOUT_H_
#define MIDIOUT_H_

#include <rtmidi/RtMidi.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class MidiOut {
public:
  MidiOut(const int port, std::condition_variable &_inAvailable,
          std::mutex &_inMutex)
      : inMutex(_inMutex), inAvailable(_inAvailable) {

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

  void run(std::queue<std::vector<unsigned char>> &midiMessages) {
    this->mainThread = std::thread([&]() {
      for (;;) {
        this->sendMessages(midiMessages);
      }
    });
  }
  void join() { this->mainThread.join(); }

private:
  std::condition_variable &inAvailable;
  std::mutex &inMutex;
  std::thread mainThread;
  RtMidiOut *midiOut;

  void sendMessages(std::queue<std::vector<unsigned char>> &messages) const {
    std::unique_lock<std::mutex> lock(inMutex);
    while (messages.empty()) {
      inAvailable.wait(lock);
    }

    auto message = messages.front();
    messages.pop();
    lock.unlock();

    std::cout << "midi out! ccn: " << +message[1] << ", ccv: " << +message[2]
              << '\n';
    for (auto &m : message) {
      this->midiOut->sendMessage(&message);
    }
  }
};

#endif // MIDIOUT
