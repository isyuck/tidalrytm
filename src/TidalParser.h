#ifndef TIDALPARSER_H_
#define TIDALPARSER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "Queue.h"
#include "QueueConsumer.h"

using state = std::array<std::array<unsigned char, 127>, 12>;

template <class OT>
class TidalParser : public QueueConsumer<osc::ReceivedMessage, OT> {
public:
  using QueueConsumer<osc::ReceivedMessage, OT>::QueueConsumer;

private:
  state currentState;
  std::mutex stateMutex;

  // turns osc into a type easier for us to use
  void main() {
    auto args = this->wait().ArgumentsBegin();
    this->pop();

    // tidal sends seconds and microsecs as two seperate ints, this converts
    // them into a std::chrono::microseconds object for ease of use
    const auto sec = std::chrono::seconds((args++)->AsInt32());
    const auto usec = std::chrono::microseconds((args++)->AsInt32());
    const std::chrono::microseconds timestamp = sec + usec;

    // skip over args we dont use yet, e.g. cycle, delta etc
    for (int i = 0; i < 5; i++) {
      args++;
    }

    // midi channel and note from next two args
    const auto chan = (args++)->AsInt32();
    // for some reason tidal sends this as a float
    const auto note = (int)(args++)->AsFloat();

    // TODO 12 mutex for each array
    stateMutex.lock();
    std::array<unsigned char, 127> prevState = currentState[chan];
    stateMutex.unlock();

    std::vector<std::vector<unsigned char>> changes;
    for (unsigned char ccn = 0; ccn < 127; ccn++) {
      const auto ccv = (unsigned char)(args++)->AsInt32();
      if (0 <= ccv && ccv <= 127) {
        if (prevState[ccn] != ccv) {
          changes.push_back(
              {(unsigned char)(0xb0 | chan), (unsigned char)(ccn + 1), ccv});
          prevState[ccn] = ccv;
        }
      }
    }
    // change note cc
    changes.push_back(
        {(unsigned char)(0xb0 | chan), 3, (unsigned char)(note + 60)});
    // note on
    changes.push_back({0x90, (unsigned char)chan, 127});

    stateMutex.lock();
    currentState[chan] = prevState;
    stateMutex.unlock();

    this->push(std::make_pair(timestamp, changes));
  }
};

#endif // TIDALPARSER_H_
