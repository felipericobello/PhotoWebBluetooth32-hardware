#ifndef CHANNEL_H
#define CHANNEL_H

#define REF_LEVEL 3500 // If channel read surpasses the reference level, then triggers timer as up or down. Stock value = 11 bits.

#include <Arduino.h>

class Channel
{
public: 
  Channel(int index, int pin);

public:
  // Reference functions
  inline bool GetUP() {return _up;}
  inline bool GetDown() {return _down;}
  inline bool IsRising() {return _signalState;}
  inline bool IsFalling() {return _signalState;}
  inline bool ShouldGetTimeStampUP() {return _getStampUP;}
  inline bool ShouldGetTimeStampDOWN() {return _getStampDOWN;}
  inline int  GetPin() {return _pin;}
  

public:
  inline void SetReferenceLevel(int referenceLevel) { _refLevel = referenceLevel; } // External change on this->Channel. 
  inline void SetMarkUP() {_up = !_up;} // Called via webserver. Must not work on OnUpdate().
  inline void SetMarkDown() {_down = !_down;} // Called via webserver. Must not work on OnUpdate().
  inline void SetStampUP(bool value) {_getStampUP = value;} // External change on this->Channel. 
  inline void SetStampDOWN(bool value) {_getStampDOWN = value;} // External change on this->Channel. 

public:
  unsigned int Read();

  // function: Flag(). If read() > _refLevel, then send flag and mark as _rise. (on rise or on fall)
private:
  /*
    Channel properties:
      Channel Index.
      Reference level: 12bits ADC on ESP32, so 0->4095.
      UP and Down check boxes.
  */

  int _channelIndex;
  int _pin;
  int _refLevel;
  bool _signalState; // needed for rise and fall timestamps.
  bool _getStampUP, _getStampDOWN;
  bool _up, _down; // Timestamp mark checkboxes

};

#endif