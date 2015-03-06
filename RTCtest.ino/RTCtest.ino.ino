#include "Wire.h"
#include "RTClib.h"
#include "Ethernet.h"
#include "SPI.h"

RTC_DS1307 RTC;

/************ ETHERNET STUFF ************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 2 };
EthernetServer server(80);

void log2serial(String msg){
    DateTime now = RTC.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print(' ');
    Serial.print(msg);
}

void setup () {
    Serial.begin(57600);
    
    //Ethernet Setup   
    Ethernet.begin(mac, ip);
    server.begin();
    
    //RTC Setup
    Wire.begin();
    RTC.begin();
    if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
 
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
}
 
void loop () {
    log2serial("Hello");
    Serial.println();
    delay(3000);
}
