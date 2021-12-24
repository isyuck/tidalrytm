#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unistd.h>

#include "oscpack/ip/UdpSocket.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/osc/OscReceivedElements.h"

#include <rtmidi/RtMidi.h>

using midiMessage = std::vector<unsigned char>;
using midiBundle = std::vector<midiMessage>;
using timestamp = std::chrono::microseconds;

std::queue<std::pair<timestamp, midiBundle>> midiQueue;
std::mutex mtx;
std::condition_variable cv;

bool logging = false;

class Listener : public osc::OscPacketListener {
public:
  Listener(float latenecy) : latency(latenecy){};
  float latency;

protected:
  virtual void ProcessMessage(const osc::ReceivedMessage &oscMessage,
                              const IpEndpointName &remoteEndpoint) {
    (void)remoteEndpoint; // suppress unused parameter warning

    try {
      auto arg = oscMessage.ArgumentsBegin();

      // we can assume these will be first
      const auto sec =
          std::chrono::seconds((arg++)->AsInt32()) +
          std::chrono::milliseconds(static_cast<long int>(latency * 1000.0f));
      const auto usec = std::chrono::microseconds((arg++)->AsInt32());
      const timestamp ts = sec + usec;

      unsigned char track = 0;
      unsigned char note = 0;
      std::vector<std::pair<unsigned char, unsigned char>> ccs;
      ccs.reserve(128);

      // parse osc args
      while (arg != oscMessage.ArgumentsEnd()) {
        switch (arg->TypeTag()) {
        case 's': {
          const std::string s = (arg++)->AsString();
          if (s.substr(0, 2) == "cc") { // cc, e.g. "cc071", or "cc119"
            ccs.push_back(
                std::make_pair(std::stoi(s.substr(2, 5)), (arg++)->AsInt32()));

          } else if (s == "track") {
            track = (arg++)->AsInt32();

          } else if (s == "n") {
            note = (arg++)->AsFloat();
          }
          break;
        }
        default: {
          arg++;
          break;
        }
        }
      }

      if (logging) {
        printf("parsed osc: track[%02d] note[%03d]", +track, +note);
        for (const auto &cc : ccs) {
          printf(" cc{%03d, %03d}", +cc.first, +cc.second);
        }
        printf("\n");
      }

      midiBundle mb;
      midiMessage mm = {0x00, 0x00, 0x00};

      if (track == 64) { // clock alias
        mm[0] = 0xF8;
        mb.push_back(mm);
      } else { // regular track

        // all ccs
        for (const auto &cc : ccs) {
          mm[0] = 0xb0 | track;
          mm[1] = cc.first;
          mm[2] = cc.second;
          mb.push_back(mm);
        }

        // note change (..?)
        mm[0] = 0xB0 | track;
        mm[1] = 0x03;
        mm[2] = 0x3C + note;
        mb.push_back(mm);

        // note on
        mm[0] = 0x90 | track;
        mm[1] = track;
        mm[2] = 0x7F;
        mb.push_back(mm);
      }

      std::unique_lock<std::mutex> lock(mtx);
      midiQueue.push(std::make_pair(ts, mb));
      lock.unlock();

    } catch (osc::Exception &e) {
      printf("\033[1;31m");
      std::cout << "error while parsing message: "
                << oscMessage.AddressPattern() << ": " << e.what() << '\n';
      printf("\033[0m");
    }
  }
};

int main(int argc, char *argv[]) {

  int opt;

  // defaults
  int midiPort = 0;
  float latency = 1; // in seconds
  int oscPort = 57120;
  const char *oscAddr = "/rytm";

  // handle cli args
  while ((opt = getopt(argc, argv, "lxm:t:d:a:h")) != -1) {
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
    case 'x':
      logging = true;
      break;
    case 'm':
      midiPort = std::atoi(optarg);
      break;
    case 'd':
      latency = std::atof(optarg);
      break;
    case 't':
      oscPort = std::atoi(optarg);
      break;
    case 'a':
      oscAddr = optarg;
      break;
    case 'h':
      std::cerr << "\nusage: " << argv[0] << " [opts]\n"
                << "-l      list midiports\n"
                << "-x      enable logging\n"
                << "-m [n]  use midiport n          (default " << midiPort
                << ") \n"
                << "-d [n]  add latency in seconds  (default " << latency
                << ") \n"
                << "-t [n]  use oscport n           (default " << oscPort
                << ") \n"
                << "-a [s]  use oscaddr s           (default " << oscAddr
                << ") \n"
                << std::endl;
      std::exit(EXIT_SUCCESS);
    }
  }

  RtMidiOut *midiOut;
  try {
    midiOut = new RtMidiOut();
    midiOut->openPort(midiPort);
  } catch (RtMidiError &error) {
    printf("\033[1;31m");
    error.printMessage();
    printf("\033[0m");
    std::exit(EXIT_FAILURE);
  }

  std::cout << "opened port #" << midiPort << " ("
            << midiOut->getPortName(midiPort) << ")\n";

  Listener listener(latency);
  UdpListeningReceiveSocket sock(
      IpEndpointName(IpEndpointName::ANY_ADDRESS, oscPort), &listener);

  std::thread sockThread = std::thread([&]() { sock.Run(); });

  std::cout << "running...\n";
  std::cout << "press Ctrl-C to quit\n";

  for (;;) {
    if (midiQueue.size() > 0) {
      const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch());

      std::unique_lock<std::mutex> lock(mtx);
      const auto front = midiQueue.front();
      lock.unlock();

      // if (logging) {
      //   auto nowCount = now.count();
      //   auto mCount = front.first.count();
      //   std::cout << "now:   " << nowCount << '\n'
      //             << "front: " << mCount << '\n'
      //             << "n - f:  " << nowCount - mCount << '\n';
      // }

      if (now >= front.first) {
        lock.lock();
        midiQueue.pop();
        lock.unlock();
        if (logging) {
          printf("sending midi messages:\n");
        }
        for (auto m : front.second) {
          midiOut->sendMessage(&m);
          if (logging) {
            printf("    {%03d, %03d, %03d} at %lld\n", +m[0], +m[1], +m[2],
                   now.count());
          }
        }
      }
    }
  }

  sockThread.join();
  std::exit(EXIT_SUCCESS);
}
