#ifndef TIDALLISTENER_H_
#define TIDALLISTENER_H_

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "Queue.h"

class TidalListener : public osc::OscPacketListener {
public:
  TidalListener(const char *_addr, Queue<osc::ReceivedMessage> &out)
      : out(out), addr(_addr) {}

private:
  Queue<osc::ReceivedMessage> &out;
  const char *addr;

protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const osc::IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    // push the msg into the queue of osc messages to be parsed (if its address
    // is the one we want)
    try {
      if (std::strcmp(oscMessage.AddressPattern(), this->addr) == 0) {
        this->out.push(oscMessage);
      }
    } catch (osc::Exception &e) {
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << "\n";
    }
  }
};

#endif // TIDALLISTENER_H_
