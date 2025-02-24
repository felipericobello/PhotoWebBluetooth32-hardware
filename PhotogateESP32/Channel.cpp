#import "Channel.h"

Channel::Channel(int index, int pin) : _channelIndex(index), _pin(pin), _refLevel(REF_LEVEL), _up(true), _down(true), _getStampUP(false), _getStampDOWN(false), _signalState(false)
{
}

unsigned int Channel::Read()
{
  unsigned int read = analogRead(_pin);

  if(read > _refLevel && !_signalState && _up)
  {
    _signalState = true;
    _getStampUP = true;
  }

  if(read < _refLevel && _signalState && _down)
  {
    _signalState = false;
    _getStampDOWN = true;
  }

  return read;
}