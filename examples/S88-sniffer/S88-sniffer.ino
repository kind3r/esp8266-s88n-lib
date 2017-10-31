/**
 * S88 ESP8266 Interface using ESP8266-S88n-lib
 **/

#include <esp8266-S88n.h>
S88nClass S88;

void setup()
{
    Serial.begin(115200);
    Serial.println("");
    Serial.println("");
    Serial.println("S88 Sniffer");
    S88.start(2);
}

void loop()
{
    S88.checkS88Data();
    yield();
}

void notifyS88Data(byte *S88data)
{
    Serial.println("New S88 data");
}