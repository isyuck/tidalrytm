#ifndef STATE_H_
#define STATE_H_

#include <array>
#include <chrono>
#include <ostream>

struct State {
  State(const unsigned char &_channel,
        const std::chrono::microseconds &_timestamp,
        const std::array<unsigned char, 127> &_cc)
      : channel(_channel), timestamp(_timestamp), cc(_cc) {}
  const unsigned char channel;
  const std::chrono::microseconds timestamp;
  const std::array<unsigned char, 127> cc;

  friend auto operator<<(std::ostream &o, State const &s) -> std::ostream & {
    o << "State:\n";
    o << "- channel: " << +s.channel << '\n';
    o << "- timestamp: " << s.timestamp.count() << '\n';
    o << "- array: \n";

    for (int i = 0; i < 127; i++) {
      o << +s.cc[i] << ' ';
    }
    o << '\n';
    return o;
  }
};

#endif // STATE_H_
