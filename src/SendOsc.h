#ifndef SENDOSC_H_
#define SENDOSC_H_

#include "Queue.h"
#include "QueueConsumer.h"

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscOutboundPacketStream.h"
#include <oscpack/ip/IpEndpointName.h>
#include <oscpack/osc/OscTypes.h>

#define OUTPUT_BUFFER_SIZE 1024

template <class IT> class SendOsc : public QueueConsumer<IT, int> {
public:
  using QueueConsumer<IT>::QueueConsumer;

  // SendOsc(std::reference_wrapper<Queue<IT>> in)
  //     : QueueConsumer<IT, int>(in),
  //       transmit(osc::IpEndpointName("127.0.0.1", 7788)) {}

  void main() {
    const auto msg = this->wait();
    this->pop();

    osc::OutboundPacketStream ps(this->buffer, OUTPUT_BUFFER_SIZE);

    ps << osc::BeginBundleImmediate << osc::BeginMessage(addr);
    for (const auto &m : msg) {
      ps << m;
    }
    ps << osc::EndMessage;
    ps << osc::EndBundle;

    this->transmit.Send(ps.Data(), ps.Size());
  }

private:
  // osc::UdpTransmitSocket transmit;
  osc::UdpTransmitSocket transmit =
      osc::UdpTransmitSocket(osc::IpEndpointName("127.0.0.1", 7788));
  char buffer[OUTPUT_BUFFER_SIZE];
  const char *addr = "/rytmvis";
};

#endif // SENDOSC_H_
