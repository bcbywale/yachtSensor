#include <ClickButton.h>
#include <LiquidCrystal.h>
#include <math.h>

 /*
  The circuit:
 * LCD RS pin to digital pin 52
 * LCD Enable pin to digital pin 53
 * LCD D4 pin to digital pin 40
 * LCD D5 pin to digital pin 39
 * LCD D6 pin to digital pin 38
 * LCD D7 pin to digital pin 37
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(42, 41, 40, 39, 38, 37);

const int numReadings = 20;
int readings[numReadings];
int index = 0;
int total = 0;
int average = 0;

int page = 1;
int rpm = 0;

float hVoltage = 0.0;
float hVoltageMax = 12.8;
float hVoltageMin = 12.0;
float hVoltagePercent = 100;
float hCapacity = 104; //amp-hours
float pExp = 1.2; //Peukert's exponent
float hAmp = 0; 

int engHours = 0;
  
float fuel = 11.5; //impliment fuel sensor - Analog
  
float  avgGPH = 0.5; //impliment calculation - requires data logging
  
int bilgeCycles = 1; //impliment bigle snesor. Add duration timer, alarm, data logging? - digital
  
String  alarms = "None"; //impliment alarms from above
  
float cabinTemp = 0;
  
float cabinHumid = 0;

const int buttonPin1 = 22;

ClickButton button1(buttonPin1, LOW, CLICKBTN_PULLUP);

void setup() {
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 4);
  page = 1; //reset page to 1 on restart
  
  for (int i = 0; i < numReadings; i++) readings[i] = 0; 
  

  button1.debounceTime = 20;
  button1.multiclickTime = 250;
  button1.longClickTime = 1000;
  
  Serial.begin(9600);
  delay(2000);
  
}

void loop() {
  button1.Update();
  
  if(button1.clicks == 1){
    lcd.clear();
    page = page + 1;
    if(page > 3) page = 1;  
  }
  
  //impliment button2 to control brightness
  analogWrite(2, 100);
 
  total= total - readings[index];         
  readings[index] = analogRead(0); 
  total= total + readings[index];       
  index = index + 1;                    
  if (index >= numReadings) index = 0;                           
  average = total / numReadings;     
  hVoltage = average* (5.0 / 1023.0)+6.5;
  Serial.println(hVoltage);
 
  hVoltagePercent = int(((hVoltage - hVoltageMin) / (hVoltageMax - hVoltageMin))*100);
  if (hVoltagePercent > 100) hVoltagePercent = 100;
  
  hAmp = 1; //impliment shunt sensor - Analog
  
  rpm =  1200; //impliment rpm sensor - Digital
  
  engHours = 104; //impliment engine hours - Digital
  
  fuel = 11.5; //impliment fuel sensor - Analog
  
  avgGPH = 0.5; //impliment calculation - requires data logging
  
  bilgeCycles = 1; //impliment bigle snesor. Add duration timer, alarm, data logging? - digital
  
  alarms = "None"; //impliment alarms from above
  

  delay(1);
  
  //Display Data 
  switch(page){
    case 1:
      lcd.setCursor(0,0);
      lcd.print("House Bank V:");
      lcd.setCursor(14,0);
      lcd.print(hVoltage);
      lcd.setCursor(0,1);
      lcd.print("House Bank A:");
      lcd.setCursor(14,1);
      lcd.print(hAmp);
      lcd.setCursor(0,2);
      lcd.print("House Bank %:");
      lcd.setCursor(14,2);
      lcd.print(hVoltagePercent);
      lcd.setCursor(0,3);
      lcd.print("Hrs Left:");
      lcd.setCursor(14,3);
      lcd.print(hCapacity / pow(hAmp,pExp));
      break;
    case 2:
      lcd.setCursor(0,0);
      lcd.print("Engine RPM:");
      lcd.setCursor(14,0);
      lcd.print(rpm);
      lcd.setCursor(0,1);
      lcd.print("Engine Hours:");
      lcd.setCursor(14,1);
      lcd.print(engHours);
      lcd.setCursor(0,2);
      lcd.print("Fuel Remaining:");
      lcd.setCursor(14,2);
      lcd.print(fuel);
      lcd.setCursor(0,3);
      lcd.print("Average GPH:");
      lcd.setCursor(14,3);
      lcd.print(avgGPH);
      break;
    case 3:
      lcd.setCursor(0,0);
      lcd.print("Cabin Temp:");
      lcd.setCursor(14,0);
      lcd.print(cabinTemp);
      lcd.setCursor(0,1);
      lcd.print("Humidty:");
      lcd.setCursor(14,1);
      lcd.print(cabinHumid);
      lcd.setCursor(0,2);
      lcd.print("Bilge Cycles:");
      lcd.setCursor(14,2);
      lcd.print(bilgeCycles);
      lcd.setCursor(0,3);
      lcd.print("Alarms:");
      lcd.setCursor(14,3);
      lcd.print(alarms);
      break;
  }
}

