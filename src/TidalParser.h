#ifndef TIDALPARSER_H_
#define TIDALPARSER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "Queue.h"
#include "TidalListener.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using state = std::array<std::array<unsigned char, 127>, 12>;

template <class OT> class TidalParser {
public:
  // create a listener and a osc socket from port, address, mutex and cond var
  // this is very ugly...
  TidalParser(const int &port, const char *addr, Queue<OT> &out)
      : listener(TidalListener<osc::ReceivedMessage>(addr, this->in)),
        sock(osc::UdpListeningReceiveSocket(
            osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, port),
            &listener)),
        out(out) {}

  void run(const int nThreads = 1) {
    // start listening for osc messages from tidal
    this->listenerThread = std::thread([&]() { this->sock.Run(); });

    // spawn n parser threads
    for (int i = 0; i < nThreads; i++) {
      this->parserThreads.push_back(std::thread([&]() { this->parse(); }));
    }
  }

  void join() {
    this->joinListener();
    std::for_each(this->parserThreads.begin(), this->parserThreads.end(),
                  [](std::thread &p) { p.join(); });
  }

private:
  // listens for osc messages
  TidalListener<osc::ReceivedMessage> listener;
  std::thread listenerThread;
  osc::UdpListeningReceiveSocket sock;
  std::vector<std::thread> parserThreads;

  // io queues
  Queue<osc::ReceivedMessage> in;
  Queue<OT> &out;
  // the state of all tracks
  state currentState;
  std::mutex stateMutex;

  // turns osc into a type easier for us to use
  void parse() {
    for (;;) {
      auto args = in.wait().ArgumentsBegin();
      in.pop();

      // tidal sends second and microsends as two seperate ints, this converts
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
        const unsigned char ccv = (unsigned char)((args++)->AsFloat() * 127);
        if (prevState[ccn] != ccv) {
          changes.push_back(
              {(unsigned char)(0xb0 | chan), (unsigned char)(ccn + 1), ccv});
          prevState[ccn] = ccv;
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

      out.push(std::make_pair(timestamp, changes));
    }
  }

  void joinListener() { this->listenerThread.join(); }
  void joinParsers() {}
};

#endif // TIDALPARSER_H_
