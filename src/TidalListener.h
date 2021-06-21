#ifndef TIDALLISTENER_H_
#define TIDALLISTENER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include <condition_variable>
#include <iostream>
#include <queue>

class TidalListener : public osc::OscPacketListener {
public:
  TidalListener(const char *_addr, std::mutex &_mut,
                std::condition_variable &_cond)
      : addr(_addr), mut(_mut), cond(_cond) {}

  bool empty() { return this->queue.empty(); }

  // get and remove the last element
  osc::ReceivedMessage pop() {
    const auto el = queue.front();
    this->queue.pop();
    return el;
  }

private:
  std::queue<osc::ReceivedMessage> queue;
  std::mutex &mut;
  std::condition_variable &cond;
  const char *addr;

protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const osc::IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    // push the msg into the queue of osc messages to be parsed (if its address
    // is the one we want)
    try {
      if (std::strcmp(oscMessage.AddressPattern(), this->addr) == 0) {
        // lock access to the queue, and add the latest message
        std::unique_lock<std::mutex> lock(mut);
        bool wasEmpty = queue.empty();
        this->queue.push(oscMessage);
        // unlock & tell a thread the queue is no longer empty
        lock.unlock();
        // std::cout << "osc message received\n";
        if (wasEmpty) {
          cond.notify_one();
        }
      }
    } catch (osc::Exception &e) {
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << "\n";
    }
  }
};

#endif // TIDALLISTENER_H_
