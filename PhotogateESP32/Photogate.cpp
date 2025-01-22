#include "Photogate.h"


Photogate::Photogate(const unsigned int* gate) : _gate(gate), _gateSize(s_gateSize), _isRunning(true)
{
  _TimeStamps = new TimeStamps();

  for(int i = 0; i < _gateSize; i++)
  {
    _Channel[i] = new Channel(i, _gate[i]);
  }

}


Photogate::~Photogate()
{
}


void Photogate::PinSetGate()
{
  for(int index = 0; index < 6; index++) {pinMode(_gate[index], INPUT_PULLUP);} // Change INPUT_PULLUP to INPUT later, check results.
  // INPUT_PULLUP: 4095 when no light is detected (must use a resistor connected to VCC, otherwise it might not work if not using devboards)
  // INPUT: 4095 when all light is detected
}


void Photogate::InitPhotogate() //Called when init button is pressed.
{
  // Start counting, call timestamps.
  // Change _isRunning status.
  _isRunning = true;
}

void Photogate::OnUpdate() // If read starts, should use this function to get time and analog data. (if using lm393, should get digital data).
{

  _TimeStamps->SetTime();
  while(Serial.available() == 0)
  {
    unsigned int t_Read[s_gateSize] = {0};
    bool rise[s_gateSize] = {false};
    bool fall[s_gateSize] = {false};

    for(int index = 0; index<s_gateSize; index++)
    {
      t_Read[index] = _Channel[index]->Read();

      rise[index] = _Channel[index]->IsRising();
      fall[index] = _Channel[index]->IsFalling();
      
      //t_Read[index] = analogRead(_gate[index]);
    }

    _TimeStamps->DeltaTime();

    for(int index = 0; index<s_gateSize; index++)
    {
      //Serial.write(t_Read[index]); // Writes analog reads on serial
      //Serial.print("Read Analog Port: "); Serial.print(index); Serial.print(t_Read[index]); Serial.print(",");


      // char strBuf[50];
      // sprintf(strBuf, "AnalogPort %d: ", index);
      // Serial.print(strBuf); Serial.print(t_Read[index]); Serial.print(",");
      
    }

    // Channel 0
    unsigned int Channel_0 = t_Read[0];
    Serial.print("Channel0:"); Serial.println(Channel_0);


    unsigned long t_dTime = _TimeStamps->GetDeltaTime();
    //Serial.write(t_dTime); // Writes deltaTime on serial
    //Serial.print("TimeStamp:"); Serial.print(t_dTime); Serial.println("us");
    
    delay(50); // debug purposes.
  }
}

