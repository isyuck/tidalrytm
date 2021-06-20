#include <chrono>
#include <iomanip>
#include <memory>
#include <ostream>
#include <vector>

struct Message {
  Message(const std::chrono::microseconds &_timestamp,
          const std::vector<std::vector<unsigned char>> &_controls)
      : timestamp(_timestamp), controls(_controls) {}

  // when to send this message
  std::chrono::microseconds timestamp;

  // vector of 3 byte arrays, representing control changes
  std::vector<std::vector<unsigned char>> controls;

  // for sorting by timestamp, sooner is lower
  bool operator<(const Message &m) const { return timestamp < m.timestamp; }

  // for cout
  friend auto operator<<(std::ostream &o, Message const &m)
      -> const std::ostream & {

    o << "  timestamp:  " << m.timestamp.count() << '\n';

    // add 0x and zero pad hex numbers
    auto format = [&o](unsigned char c) -> void {
      o << " 0x" << std::setw(2) << std::setfill('0') << (int)c;
    };

    int i = 0;
    for (const auto &cc : m.controls) {
      // cout with hex
      o << "  control[" << i << "]:" << std::hex;
      format(cc[0]);
      format(cc[1]);
      format(cc[2]);
      i++;
      o << '\n' << std::dec; // return to decimal
    }
    return o;
  }
};

class MessageQueue {
public:
  std::vector<Message> &get() { return this->messages; }

  // construct and add a message from a timestamp and a vector of controls
  void push(const std::chrono::microseconds &timestamp,
            const std::vector<std::vector<unsigned char>> &controls) {
    messages.push_back(Message(timestamp, controls));
  }

  // for cout
  friend auto operator<<(std::ostream &o, MessageQueue const &mq)
      -> const std::ostream & {
    int i = 0;
    for (const auto &m : mq.messages) {
      o << "---------------------------------\nmessage[" << i << "]:\n" << m;
      i++;
    }
    return o << '\n';
  }

private:
  std::vector<Message> messages;
};
