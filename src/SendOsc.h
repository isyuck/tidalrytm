#ifndef SENDOSC_H_
#define SENDOSC_H_

#include "Queue.h"

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscOutboundPacketStream.h"
#include <oscpack/osc/OscTypes.h>

#define OUTPUT_BUFFER_SIZE 1024

class SendOsc {
public:
  SendOsc(const int &port, const char *addr)
      : transmit(osc::IpEndpointName("127.0.0.1", port)), addr(addr) {}

  void send() {
    osc::OutboundPacketStream ps(this->buffer, OUTPUT_BUFFER_SIZE);
    ps << osc::BeginMessage(addr) << true << osc::EndMessage;

    transmit.Send(ps.Data(), ps.Size());
  }

private:
  osc::UdpTransmitSocket transmit;
  char buffer[OUTPUT_BUFFER_SIZE];
  const char *addr;
};

#endif // SENDOSC_H_
