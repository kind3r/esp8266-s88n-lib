/*
  S88 ESP8266 Interface
  
  S88 zum Anschluss von bis zu 31 Modulen mit 16 Eingängen zur Verfügung.
    
  Software-Version:
*/  #define Version "1.00"
#define VersionDate "23.10.14"
/*  
  Versionsstatistik:

  v1.00 (2014)
        Erste lauffähige S88 Version.
        Rückmeldung über Serial.

   Author: */ 
  #define Author "2014 Philipp Gahtow"


//Timer frequency is 250kHz for ( /64 prescale from 16MHz )
#define TIMER_Time 0x80 //je größer desto schneller die Abfrageintervalle
/*
  Der Timer erzeugt den notwendigen Takt für die S88 Schiebeabfragen.
  Je nach verwendten Modulen kann der Takt beliebigt in seiner Geschwindigkeit
  geändert werden, aber nicht jede Hardware unterstützt ein "fast" Auslesen!
*/
  

byte S88Module = 2;    //Anzahl der 8x Module - EEPROM 101
                        //maximal 62 x 8 Port Module = 31 Module à 16 Ports

byte data[62];     //Zustandsspeicher

//--------------------------------------------------------------
//Pinbelegungen am Dekoder:
  //Eingänge:
#define S88DataPin 15//A0      //S88 Data IN
  //Ausgänge:
#define S88ClkPin 13//A1    //S88 Clock
#define S88PSPin 12//A2    //S88 PS/LOAD
#define S88ResetPin 14//A3    //S88 Reset

int S88RCount = 0;    //Lesezähler 0-39 Zyklen
int S88RMCount = 0;   //Lesezähler Modul-Pin

/*
'0' = keine
's' = Änderungen vorhanden, noch nicht fertig mit Auslesen
'i' = Daten vollständig, senden an PC
*/
char S88sendon = '0';        //Bit Änderung

extern "C" {
#include "user_interface.h"
}
os_timer_t myTimer;

//-------------------------------------------------------------- 
//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.
void SetupTimer2() {
  os_timer_setfn(&myTimer, S88Timer, NULL);
  os_timer_arm(&myTimer, 1, true);
}

//-------------------------------------------------------------- 
void setup()
{

  Serial.begin(115200);
  Serial.println("S88 Bus");
  
  pinMode(S88ResetPin, OUTPUT);    //Reset
  pinMode(S88PSPin, OUTPUT);      //PS/LOAD
  pinMode(S88ClkPin, OUTPUT);      //Clock
  digitalWrite(S88ResetPin, LOW);
  digitalWrite(S88PSPin, LOW);      //init
  digitalWrite(S88ClkPin, LOW);
  
  pinMode(S88DataPin, INPUT);    //Dateneingang

  SetupTimer2();    //Clock für S88 abfragen.
}


//-------------------------------------------------------------- 
void loop()
{ 
  
  if (S88sendon == 'i') {
    for(byte m = 0; m < S88Module; m++) {
        byte lowOut = data[m];
        Serial.print("M");
        Serial.print(m);
        Serial.print(":, ");
        Serial.print(lowOut, BIN);
        Serial.print("; ");
    }
    Serial.println();  //carriage return 
    S88sendon = '0';        //Speicher Rücksetzten
  }
  yield();
}

//-------------------------------------------------------------- 
//Timer ISR Routine
//Timer2 overflow Interrupt vector handler
//ISR(TIMER2_OVF_vect) {
void S88Timer(void *pArg) {
  if (S88RCount == 3)    //Load/PS Leitung auf 1, darauf folgt ein Schiebetakt nach 10 ticks!
    digitalWrite(S88PSPin, HIGH);
  if (S88RCount == 4)   //Schiebetakt nach 5 ticks und S88Module > 0
    digitalWrite(S88ClkPin, HIGH);       //1. Impuls
  if (S88RCount == 5)   //Read Data IN 1. Bit und S88Module > 0
    S88readData();    //LOW-Flanke während Load/PS Schiebetakt, dann liegen die Daten an
  if (S88RCount == 9)    //Reset-Plus, löscht die den Paralleleingängen vorgeschaltetetn Latches
    digitalWrite(S88ResetPin, HIGH);
  if (S88RCount == 10)    //Ende Resetimpuls
    digitalWrite(S88ResetPin, LOW);
  if (S88RCount == 11)    //Ende PS Phase
    digitalWrite(S88PSPin, LOW);
  if (S88RCount >= 12 && S88RCount < 10 + (S88Module * 8) * 2) {    //Auslesen mit weiteren Schiebetakt der Latches links
    if (S88RCount % 2 == 0)      //wechselnder Taktimpuls/Schiebetakt
      digitalWrite(S88ClkPin, HIGH);  
    else S88readData();    //Read Data IN 2. bis (Module*8) Bit
  }
  S88RCount++;      //Zähler für Durchläufe/Takt
  if (S88RCount >= 10 + (S88Module * 8) * 2) {  //Alle Module ausgelesen?
    S88RCount = 0;                    //setzte Zähler zurück
    S88RMCount = 0;                  //beginne beim ersten Modul von neuem
    //init der Grundpegel
    digitalWrite(S88PSPin, LOW);    
    digitalWrite(S88ClkPin, LOW);
    digitalWrite(S88ResetPin, LOW);
    if (S88sendon == 's')  //Änderung erkannt
      S88sendon = 'i';      //senden
  }
  //Capture the current timer value. This is how much error we have due to interrupt latency and the work in this function
//  TCNT2 = TCNT2 + TIMER_Time;    //Reload the timer and correct for latency.
}

//--------------------------------------------------------------
//Einlesen des Daten-Bit und Vergleich mit vorherigem Durchlauf
void S88readData() {
  digitalWrite(S88ClkPin, LOW);  //LOW-Flanke, dann liegen die Daten an
  byte Modul = S88RMCount / 8;
  byte Port = S88RMCount % 8;
  byte getData = digitalRead(S88DataPin);  //Bit einlesen
  if (bitRead(data[Modul],Port) != getData) {     //Zustandsänderung Prüfen?
    bitWrite(data[Modul],Port,getData);          //Bitzustand Speichern
    S88sendon = 's';  //Änderung vorgenommen. (SET)
  }
  S88RMCount++;
}

