#ifndef TIDALPARSER_H_
#define TIDALPARSER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "TidalListener.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct State {
  State(const unsigned char &_channel) : channel(_channel) {}
  const unsigned char channel;
  std::chrono::microseconds timestamp;
  std::array<unsigned char, 127> cc;

  friend auto operator<<(std::ostream &o, State const &s) -> std::ostream & {
    o << "State:\n";
    o << "- channel: " << +s.channel << '\n';
    o << "- timestamp: " << s.timestamp.count() << '\n';
    return o;
  }
};

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

  void run() {
    // start listening for osc messages from tidal
    this->listenerThread = std::thread([&]() { this->sock.Run(); });

    // spawn n parser threads
    for (int i = 0; i < 8; i++) {
      this->parserThreads.push_back(std::thread([&]() { this->parse(); }));
    }
  }

  void join() {
    this->joinListener();
    this->joinParsers();
  }

  State pop() {
    const auto el = stateQueue.front();
    this->stateQueue.pop();
    return el;
  }

  bool empty() { return this->stateQueue.empty(); }

private:
  TidalListener listener;
  std::thread listenerThread;
  std::vector<std::thread> parserThreads;
  std::queue<State> stateQueue;
  std::mutex oscMutex;
  std::condition_variable oscToParse;
  std::mutex &statesMutex;
  std::condition_variable &statesAvailable;
  osc::UdpListeningReceiveSocket sock;

  void joinListener() { this->listenerThread.join(); }
  void joinParsers() {
    std::for_each(this->parserThreads.begin(), this->parserThreads.end(),
                  [](std::thread &p) { p.join(); });
  }

  void parse() {
    for (;;) {
      // wait for something to parse
      std::unique_lock<std::mutex> oscLock(oscMutex);
      while (this->listener.empty()) {
        oscToParse.wait(oscLock);
      }
      // the parsing happens here

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

      State state(chan);
      state.timestamp = timestamp;

      std::unique_lock<std::mutex> stateLock(statesMutex);
      bool wasEmpty = stateQueue.empty();
      this->stateQueue.push(state);
      stateLock.unlock();
      if (wasEmpty) {
        statesAvailable.notify_one();
      }
    }
  }
};

#endif // TIDALPARSER_H_
