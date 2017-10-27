
#ifndef S88n_h
#define S88n_h

// include types & constants of Wiring core API
#if defined(WIRING)
#include <Wiring.h>
#elif ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// PIN configuration
#ifndef S88DataPin
#define S88DataPin 15 //S88 Data IN
#endif
#ifndef S88ClkPin
#define S88ClkPin 13 //S88 Clock
#endif
#ifndef S88PSPin
#define S88PSPin 12 //S88 PS/LOAD
#endif
#ifndef S88ResetPin
#define S88ResetPin 14 //S88 Reset
#endif

extern "C" {
#include "user_interface.h"
}

class S88nClass
{
public:
  S88nClass(void);
  bool start(uint8_t modules); // start S88 bus with the number of modules specified
  bool stop();                 // stop S88 bus
  void checkS88Data();         // check if the S88 data has changed and notify
  void getData(byte *data);    // get current S88 data

  // public for calling from timer interrupt
  void S88ISR();            // Interrupt Service Routine
  static S88nClass *object; // for calling from ISR

private:
  void S88readData(); // Read data byte from S88

  uint8_t S88Module;  // number of 8 port S88 modules
  uint8_t S88RCount;  // counter for the number of reading steps (clock, reset, ps pin changes)
  uint8_t S88RMCount; // counter for the number of read bits

  // statuses
  byte S88Status;
  static const byte S88DATA_INIT = 0;      // data is being initialized, cannot use in this state
  static const byte S88DATA_UNCHANGED = 1; // data is unchanged
  static const byte S88DATA_READING = 2;   // data is changing and reading is in progress
  static const byte S88DATA_CHANGED = 3;   // data has changed and must notify

  byte *S88data; //data buffer
  os_timer_t S88Timer;
};

// #if defined(__cplusplus)
extern "C" {
// #endif

extern void notifyS88Data(byte *S88data) __attribute__((weak)); // callback function for S88 data change

// #if defined(__cplusplus)
}
// #endif

#endif