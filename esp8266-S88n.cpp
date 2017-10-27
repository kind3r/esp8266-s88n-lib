/*
*****************************************************************************
  * esp8266-S88n.h - library for using S88n on ESP8266
  *
*****************************************************************************
*/
#include "esp8266-S88n.h"

S88nClass *S88nClass::object = 0;

S88nClass::S88nClass() 
{
  S88Module = 0;
  S88RCount = 0;
  S88RMCount = 0;
  S88Status = S88nClass::S88DATA_INIT;
  object = this;
}

void S88ISRProxy(void *pArg) {
  if(S88nClass::object) {
    S88nClass::object->S88ISR();
  }
}  
  
bool S88nClass::start(uint8_t modules)
{
  if(S88Status != S88nClass::S88DATA_INIT) {
    return false;
  }
  S88Module = modules;
  if (S88Module > 62 || S88Module == 0)
  { //S88 off!
    S88Module = 0;
    return false;
  }

  // Setup data buffer
  S88data = (byte *)malloc(S88Module);
  for(byte i = 0; i < S88Module; i++) {
    S88data[i] = 0;
  }
  if(S88data != NULL) {
    // Setup timer
    os_timer_setfn(&S88Timer, S88ISRProxy, NULL);
    // Start timer
    os_timer_arm(&S88Timer, 1, true);
    S88Status = S88nClass::S88DATA_UNCHANGED;
    return true;
  } else {
    return false;
  }
}

bool S88nClass::stop()
{
  // Stop timer
  os_timer_disarm(&S88Timer);
  // Dealocate data buffer
  free(S88data);
  // Update status
  S88Status = S88nClass::S88DATA_INIT;
}

void S88nClass::getData(byte *data)
{
  data = S88data;
}

//--------------------------------------------------------------
//S88 ISR Routine
void S88nClass::S88ISR()
{
  if (S88RCount == 3) //Load/PS Leitung auf 1, darauf folgt ein Schiebetakt nach 10 ticks!
    digitalWrite(S88PSPin, HIGH);
  if (S88RCount == 4)              //Schiebetakt nach 5 ticks und S88Module > 0
    digitalWrite(S88ClkPin, HIGH); //1. Impuls
  if (S88RCount == 5)              //Read Data IN 1. Bit und S88Module > 0
    S88readData();                 //LOW-Flanke während Load/PS Schiebetakt, dann liegen die Daten an
  if (S88RCount == 9)              //Reset-Plus, löscht die den Paralleleingängen vorgeschaltetetn Latches
    digitalWrite(S88ResetPin, HIGH);
  if (S88RCount == 10) //Ende Resetimpuls
    digitalWrite(S88ResetPin, LOW);
  if (S88RCount == 11) //Ende PS Phase
    digitalWrite(S88PSPin, LOW);
  if (S88RCount >= 12 && S88RCount < 10 + (S88Module * 8) * 2)
  {                         //Auslesen mit weiteren Schiebetakt der Latches links
    if (S88RCount % 2 == 0) //wechselnder Taktimpuls/Schiebetakt
      digitalWrite(S88ClkPin, HIGH);
    else
      S88readData(); //Read Data IN 2. bis (Module*8) Bit
  }
  S88RCount++; //Zähler für Durchläufe/Takt
  if (S88RCount >= 10 + (S88Module * 8) * 2)
  {                 //Alle Module ausgelesen?
    S88RCount = 0;  //setzte Zähler zurück
    S88RMCount = 0; //beginne beim ersten Modul von neuem
    //init der Grundpegel
    digitalWrite(S88PSPin, LOW);
    digitalWrite(S88ClkPin, LOW);
    digitalWrite(S88ResetPin, LOW);
    // if data has changed set status acordingly
    if (S88Status == S88nClass::S88DATA_READING) {
      S88Status = S88nClass::S88DATA_CHANGED;
    }
  }
}

//--------------------------------------------------------------
//Read the data bit, compare with previous and update if needed
void S88nClass::S88readData()
{
  digitalWrite(S88ClkPin, LOW); //LOW-Flanke, dann liegen die Daten an
  byte Modul = S88RMCount / 8;
  byte Port = S88RMCount % 8;
  byte getData = digitalRead(S88DataPin); //read the bit for the corresponding port
  // if the port status is changed, modify and update status
  if (bitRead(S88data[Modul], Port) != getData)
  {
    bitWrite(S88data[Modul], Port, getData);
    S88Status == S88nClass::S88DATA_READING;
  }
  S88RMCount++;
}

//--------------------------------------------------------------------------------------------
//Check if S88 data has changed and notify
void S88nClass::checkS88Data()
{
  if (S88Status)
  {
    if(notifyS88Data) {
      notifyS88Data(S88data);
      S88Status = S88nClass::S88DATA_UNCHANGED;
    }
  }
}