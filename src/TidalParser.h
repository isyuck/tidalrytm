#ifndef TIDALPARSER_H_
#define TIDALPARSER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "State.h"
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
#include <vector>

class TidalParser {
public:
  // create a listener and a osc socket from port, address, mutex and cond var
  // this is very ugly...
  TidalParser(const int &port, const char *addr,
              std::condition_variable &_statesAvailable,
              std::mutex &_statesMutex)
      : listener(TidalListener(addr, this->oscMutex, this->oscToParse)),
        sock(osc::UdpListeningReceiveSocket(
            osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, port),
            &listener)),
        statesAvailable(_statesAvailable), statesMutex(_statesMutex) {}

  void run(std::queue<State> &stateQueue) {
    // start listening for osc messages from tidal
    this->listenerThread = std::thread([&]() { this->sock.Run(); });

    // spawn n parser threads
    for (int i = 0; i < 8; i++) {
      this->parserThreads.push_back(
          std::thread([&]() { this->parse(stateQueue); }));
    }
  }

  void join() {
    this->joinListener();
    this->joinParsers();
  }

  // State pop() {
  //   const auto el = stateQueue.front();
  //   std::cout << "state queue has " << stateQueue.size() << "elements\n";
  //   this->stateQueue.pop();
  //   return el;
  // }

  // bool empty() { return this->stateQueue.empty(); }

private:
  // listens for osc messages
  TidalListener listener;
  std::thread listenerThread;
  osc::UdpListeningReceiveSocket sock;
  // turns osc messages into states and appends them to the stateQueue,
  // multithreaded
  std::vector<std::thread> parserThreads;
  // for locking and notifying waiting threads
  std::mutex oscMutex;
  std::condition_variable oscToParse;
  std::mutex &statesMutex;
  std::condition_variable &statesAvailable;

  void joinListener() { this->listenerThread.join(); }
  void joinParsers() {
    std::for_each(this->parserThreads.begin(), this->parserThreads.end(),
                  [](std::thread &p) { p.join(); });
  }

  void parse(std::queue<State> &stateQueue) {
    for (;;) {
      // wait for something to parse
      std::unique_lock<std::mutex> oscLock(oscMutex);
      while (this->listener.empty()) {
        // std::cout << "waiting for osc messages...\n";
        oscToParse.wait(oscLock);
      }
      // the parsing happens here
      // std::cout << "parsing osc message!\n";

      // get a message's args
      auto args = this->listener.pop().ArgumentsBegin();
      oscLock.unlock();

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

      // pull all control changes into an array
      std::array<unsigned char, 127> cc;
      for (int i = 0; i < 127; i++) {
        // std::vector<unsigned char> cc{0xb0 | chan, i + 1, v};
        cc[i] = (unsigned char)(args++)->AsInt32();
      }

      // add to the state queue and notify any threads waiting for new states
      std::unique_lock<std::mutex> stateLock(statesMutex);
      bool wasEmpty = stateQueue.empty();
      stateQueue.push(State(chan, timestamp, cc));
      stateLock.unlock();
      if (wasEmpty) {
        statesAvailable.notify_one();
      }
    }
  }
};

#endif // TIDALPARSER_H_
