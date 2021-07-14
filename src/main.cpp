#include "oscpack/osc/OscReceivedElements.h"

#include "MidiOut.h"
#include "Queue.h"
#include "Scheduler.h"
#include "TidalListener.h"
#include "TidalParser.h"

#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

using tidalMessage = std::pair<std::chrono::microseconds,
                               std::vector<std::vector<unsigned char>>>;

using midiMessage = std::vector<unsigned char>;

int main(int argc, char **argv) {

  int opt;

  // defaults
  int midiPort = 1;
  int oscPort = 57120;
  const char *oscAddr = "/rytm";

  // handle cli args
  while ((opt = getopt(argc, argv, "lm:t:a:h")) != -1) {
    switch (opt) {
    // list midi out ports
    case 'l': {
      // temporary midiout
      RtMidiOut *m = new RtMidiOut();
      std::string name = "";
      const auto count = m->getPortCount();
      std::cout << count << " out ports:\n";
      for (unsigned int i = 0; i < count; i++) {
        try {
          name = m->getPortName(i);
        } catch (RtMidiError &error) {
          error.printMessage();
        }
        std::cout << " #" << i << ": " << name << '\n';
      }
      std::exit(EXIT_FAILURE);
    }
    case 'm':
      midiPort = std::atoi(optarg);
      break;
    case 't':
      oscPort = std::atoi(optarg);
      break;
    case 'a':
      oscAddr = optarg;
      break;
    case 'h':
      std::cerr << "usage: " << argv[0] << " [opts]\n"
                << "-l      list midiports\n"
                << "-m [n]  use midiport n (default 1) \n"
                << "-t [n]  use oscport n (default 57120) \n"
                << "-a [s]  use oscaddr s (default \"/rytm\") \n"
                << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  Queue<osc::ReceivedMessage> oscMessages;
  Queue<tidalMessage> tidalMessages;
  Queue<midiMessage> midiMessages;
  Queue<midiMessage> visMessages;

  // listen for osc from tidal
  TidalListener tidalListener(oscAddr, oscMessages);

  osc::UdpListeningReceiveSocket sock(
      osc::IpEndpointName(osc::IpEndpointName::ANY_ADDRESS, oscPort),
      &tidalListener);

  // parse osc into a timestamped 'midi' message
  TidalParser<tidalMessage> tidalParser(oscMessages, tidalMessages);

  // pulls from the midi messages and send to midiout queue when appropriate
  Scheduler<tidalMessage, midiMessage> scheduler(tidalMessages, midiMessages);

  // immediately sends any messages in the midiMessage queue to the rytm
  MidiOut<midiMessage> midiOut(midiMessages);
  midiOut.openPort(midiPort);

  std::thread sockThread = std::thread([&]() { sock.Run(); });
  // arg is n threads
  tidalParser.run(8);
  scheduler.run(8);
  midiOut.run(1);

  sockThread.join();
  tidalParser.join();
  scheduler.join();
  midiOut.join();

  std::exit(EXIT_SUCCESS);
}
