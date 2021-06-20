#ifndef TRACK_H_
#define TRACK_H_

#include <array>

class Track {
public:
  Track(const unsigned char &_number) : number(_number) {}
  // aka channel
  const unsigned char number;
  // compare the current state with a new state
  bool stateChanged(std::array<unsigned char, 127> &newState) const {
    return newState == this->state;
  }
  // access this tracks state
  std::array<unsigned char, 127> &getStateRef() { return this->state; }

private:
  // the current state of this track, represented as control change values
  // TODO initialise this from a midi sysex dump
  std::array<unsigned char, 127> state;
};

#endif // TRACK_H_
