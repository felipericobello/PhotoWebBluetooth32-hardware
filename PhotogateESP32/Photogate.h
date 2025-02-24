#ifndef PHOTOGATE_H
#define PHOTOGATE_H

/*
Classe de controle das funções do photogate. Reponsável por juntar os recursos envolvendo canais, temporizadores, webserver etc.
*/

#include <Arduino.h>
#include "Timestamps.h"
#include "Channel.h"

class Photogate
{
public:
  //Initialize functions
  Photogate(const unsigned int* gate);
  ~Photogate();
  void PinSetGate();
  void InitPhotogate();
  void OnUpdate();

public:
  inline int GetGateSize() {return _gateSize;}
  inline int GetGate(int index) {return _gate[index];} // 0 to n
  inline bool IsRunning() {return _isRunning;}

public:
  

private:
  const unsigned int* _gate; 
  const unsigned int  _gateSize;
  bool _isRunning;

private: //static allocations
  static const int s_gateSize = 1; // "As a result, the declaration of a static data member is not considered a definition. The data member is declared in class scope, but definition is performed at file scope."

private: // plugins
  TimeStamps* _TimeStamps;
  Channel* _Channel[s_gateSize]; // Ideally, I would like to use std::vector<> here, but that could cause some memory issues in microcontrollers, as stack overflow.
};


#endif