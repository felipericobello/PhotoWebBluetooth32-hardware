#import "Channel.h"

Channel::Channel(int index, int pin) : _channelIndex(index), _pin(pin), _refLevel(REF_LEVEL), _up(true), _down(false)
{
}

unsigned int Channel::Read()
{
  unsigned int read = analogRead(_pin);
  if(read > _refLevel and _rise == false)
  {
    _rise = true;
    _fall = false;
  }

  if(read > _refLevel and _fall == false)
  {
    _rise = false;
    _fall = true;
  }

  return read;
}