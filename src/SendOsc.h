#ifndef SENDOSC_H_
#define SENDOSC_H_

#include "Queue.h"

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscOutboundPacketStream.h"
#include <oscpack/osc/OscTypes.h>

#include <thread>

#define OUTPUT_BUFFER_SIZE 1024

template <class IT> class SendOsc {
public:
  SendOsc(const int &port, const char *addr, Queue<IT> &in)
      : transmit(osc::IpEndpointName("127.0.0.1", port)), addr(addr), in(in),
        ps(this->buffer, OUTPUT_BUFFER_SIZE) {}

  void run() {
    this->thread = std::thread([&]() {
      for (;;) {

        const auto msg = in.wait();
        in.pop();

        ps << osc::BeginMessage(addr);
        for (const auto &m : msg) {
          ps << m;
        }
        ps << osc::EndMessage;

        this->transmit.Send(ps.Data(), ps.Size());
        this->ps.Clear();
      }
    });
  }

private:
  Queue<IT> &in;
  osc::UdpTransmitSocket transmit;
  char buffer[OUTPUT_BUFFER_SIZE];
  osc::OutboundPacketStream ps;
  const char *addr;
  std::thread thread;
};

#endif // SENDOSC_H_
