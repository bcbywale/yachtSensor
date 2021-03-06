#include <Wire.h>
#include <math.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>
#include <SPI.h>

#include <LiquidCrystal.h>
#include "RTClib.h"
#include <ClickButton.h>

#include "yachtSensor.h"
/************ REALTIME CLOCK STUFF ************/
RTC_DS1307 RTC;

/************ ETHERNET STUFF ************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 2 };
EthernetServer server(80);

/************ SDCARD STUFF ************/
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
SdFile dataFile;
const int SSpin = 53;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

/************ LCD STUFF ************/
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(29, 27, 40, 39, 38, 37);

/************ BUTTON STUFF ************/
const int buttonPin1 = 22;
ClickButton button1(buttonPin1, LOW, CLICKBTN_PULLUP);

/************ PROGRAM CONSTANT STUFF ************/
const int numReadings = 20;
const long interval = 1000; //Controls the update frequency of the screen and serial output
#define BUFSIZ 100 // How big our line buffer should be. 100 is plenty!

/************ PROGRAM VARIABLES STUFF ************/

int readings[numReadings];
int index = 0;
int total = 0;
int average = 0;

int page = 3;
int rpm = 0;

float hVoltage = 0.0;
float hVoltageMax = 12.8;
float hVoltageMin = 12.0;
int hVoltagePercent = 100;
float hCapacity = 104; //amp-hours
float pExp = 1.2; //Peukert's exponent
float hAmp = 0;

float sVoltage = 0.0; 
float sVoltageMax = 12.8;
float sVoltageMin = 12.0;

int engHours = 0;
float fuel = 11.5; //impliment fuel sensor - Analog
float fuelV = 0;
float fuelO = 0;
float  avgGPH = 0.5; //impliment calculation - requires data logging
int bilgeCycles = 1; //impliment bigle snesor. Add duration timer, alarm, data logging? - digital
String  alarms = "None"; //impliment alarms from above
float cabinTemp = 0;
float cabinHumid = 0;
unsigned long previousMillis = 0;


void ListFiles(EthernetClient client, uint8_t flags)
{
	// This code is just copied from SdFile.cpp in the SDFat library
	// and tweaked to print to the client output in html!
	dir_t p;

	root.rewind();
	client.println("<ul>");
	while (root.readDir(&p) > 0)
	{
	// done if past last used entry
	if (p.name[0] == DIR_NAME_FREE) break;

	// skip deleted entry and entries for . and  ..
	if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

	// only list subdirectories and files
	if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

	// print any indent spaces
	client.print("<li><a href=\"");
	for (uint8_t i = 0; i < 11; i++)
	{
		if (p.name[i] == ' ') continue;
		if (i == 8)
		{
			client.print('.');
		}
		client.print((char)p.name[i]);
	}
	client.print("\">");

	// print file name with possible blank fill
	for (uint8_t i = 0; i < 11; i++)
	{
		if (p.name[i] == ' ') continue;
		if (i == 8)
		{
			client.print('.');
		}
		client.print((char)p.name[i]);
	}

	client.print("</a>");

	if (DIR_IS_SUBDIR(&p))
	{
		client.print('/');
	}

	// print modify date/time if requested
	if (flags & LS_DATE)
	{
		root.printFatDate(p.lastWriteDate);
		client.print(' ');
		root.printFatTime(p.lastWriteTime);
	}
	// print size if requested
	if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE))
	{
		client.print(' ');
		client.print(p.fileSize);
	}
	client.println("</li>");
	}
	client.println("</ul>");
}

void error_P(const char* str)
{
	PgmPrint("error: ");
	SerialPrintln_P(str);
	if (card.errorCode())
	{
		PgmPrint("SD error: ");
		Serial.print(card.errorCode(), HEX);
		Serial.print(',');
		Serial.println(card.errorData(), HEX);
	}
	while (1);
}

void setup()
{
	Serial.begin(9600);
	Wire.begin();
	RTC.begin();
	//RTC.adjust(DateTime(__DATE__, __TIME__));
	pinMode(8,  OUTPUT);
	analogWrite(8, 25);

	// initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
	// breadboards.  use SPI_FULL_SPEED for better performance.
	pinMode(53, OUTPUT);                       // set the SS pin as an output (necessary!)
	digitalWrite(53, HIGH);                    // but turn off the W5100 chip!
	if (!card.init(SPI_HALF_SPEED, 4)) error("card.init failed!");
	// initialize a FAT volume
	if (!volume.init(&card)) error("vol.init failed!");
	if (!root.openRoot(&volume)) error("openRoot failed");

	Ethernet.begin(mac, ip);
	server.begin();

	// set up the LCD's number of columns and rows:
	lcd.begin(20, 4);
	page = 1; //reset page to 1 on restart

	for (int i = 0; i < numReadings; i++) readings[i] = 0;

	button1.debounceTime = 10;
	button1.multiclickTime = 250;
	button1.longClickTime = 1500;

}

void loop()
{
	char clientline[BUFSIZ];
	int index = 0;

	DateTime now = RTC.now();
	unsigned long currentMillis = millis();
	
	EthernetClient client = server.available();
	if (client)
	{
		// an http request ends with a blank line
		boolean current_line_is_blank = true;

		// reset the input buffer
		index = 0;

		while (client.connected())
		{
			if (client.available())
			{
				char c = client.read();

		// If it isn't a new line, add the character to the buffer
		if (c != '\n' && c != '\r')
		{
			clientline[index] = c;
			index++;
		// are we too big for the buffer? start tossing out data
		if (index >= BUFSIZ)
		index = BUFSIZ - 1;

		// continue to read more data!
		continue;
		}

		// got a \n or \r new line, which means the string is done
		clientline[index] = 0;

		// Print it out for debugging
		Serial.println(clientline);

		// Look for substring such as a request to get the root file
		if (strstr(clientline, "GET / ") != 0)
		{
		// send a standard http response header
		client.println("HTTP/1.1 200 OK");
		client.println("Content-Type: text/html");
		client.println();

		// print all the files, use a helper to keep it clean
		client.println("<h2>Files:</h2>");
		ListFiles(client, LS_SIZE);
		}
		else if (strstr(clientline, "GET /") != 0)
		{
		// this time no space after the /, so a sub-file!
		char *filename;

		filename = clientline + 5; // look after the "GET /" (5 chars)
		// a little trick, look for the " HTTP/1.1" string and
		// turn the first character of the substring into a 0 to clear it out.
		(strstr(clientline, " HTTP"))[0] = 0;

		// print the file we want
		Serial.println(filename);

		if (! file.open(&root, filename, O_READ))
		{
			client.println("HTTP/1.1 404 Not Found");
			client.println("Content-Type: text/html");
			client.println();
			client.println("<h2>File Not Found!</h2>");
			break;
		}

		Serial.println("Opened!");

		client.println("HTTP/1.1 200 OK");
		client.println("Content-Type: text/plain");
		client.println();

		int16_t c;
		while ((c = file.read()) > 0)
		{
		// uncomment the serial to debug (slow!)
		//Serial.print((char)c);
		client.print((char)c);
		}
		file.close();
		}
		else
		{
		// everything else is a 404
		client.println("HTTP/1.1 404 Not Found");
		client.println("Content-Type: text/html");
		client.println();
		client.println("<h2>File Not Found!</h2>");
		}
		break;
		}
		}
		// give the web browser time to receive the data
		delay(1);
		client.stop();
	}

	button1.Update();
	
	if (button1.clicks != 0)
	{
		lcd.clear();
		page = page + 1;
		if (page > 3) page = 1;
	}

	//impliment button2 to control brightness
	analogWrite(2, 100);

	total = total - readings[index];
	readings[index] = analogRead(0);
	total = total + readings[index];
	index = index + 1;
	if (index >= numReadings) index = 0;
	average = total / numReadings;
	
	hVoltage = (analogRead(15) * (5.0 / 1023.0)) * (5.0405282035);
	
	hVoltagePercent = int(((hVoltage - hVoltageMin) / (hVoltageMax - hVoltageMin)) * 100);
	if (hVoltagePercent > 100) hVoltagePercent = 100;
	
	hAmp = ((analogRead(13) * (5.0 / 1023.0))-2.5)*(50/2.5);
  
        sVoltage = (analogRead(14) * (5.0 / 1023.0)) * (5.0405282035);
        
	rpm =  1200; //impliment rpm sensor - Digital

	engHours = 104; //impliment engine hours - Digital

	fuelV = analogRead(3) * (5.0 / 1023.0);
	fuelO = ((fuelV * 500.0) / (5.0 - fuelV));
	fuel = 11.5 * (1.0 - ((fuelO - 33.0) / 207.0));
	if (fuel < 0) fuel = 0;

	avgGPH = 0.5; //impliment calculation - requires data logging

	bilgeCycles = 1; //impliment bigle snesor. Add duration timer, alarm, data logging? - digital

	alarms = "None"; //impliment alarms from above

	if (currentMillis - previousMillis >= interval) 
	{
		// save the last time the loop was entered
		previousMillis = currentMillis;
		
		//output data
		dataFile.open(&root, "datalog.txt", FILE_WRITE);
		dataFile.print(now.year(), DEC);
		dataFile.print("/");
		dataFile.print(now.month(), DEC);
		dataFile.print("/");
		dataFile.print(now.day(), DEC);
		dataFile.print(" ");
		dataFile.print(now.hour(), DEC);
		dataFile.print(":");
		dataFile.print(now.minute(), DEC);
		dataFile.print(":");
		dataFile.print(now.second(), DEC);
		dataFile.print("|");
		dataFile.print(hVoltage);
		dataFile.print("|");
		dataFile.print(sVoltage);
		dataFile.print("\n");
		dataFile.close();

		//echo to serial
		Serial.print(now.year(), DEC);
		Serial.print("/");
		Serial.print(now.month(), DEC);
		Serial.print("/");
		Serial.print(now.day(), DEC);
		Serial.print(" ");
		Serial.print(now.hour(), DEC);
		Serial.print(":");
		Serial.print(now.minute(), DEC);
		Serial.print(":");
		Serial.print(now.second(), DEC);
		Serial.print("|");
		Serial.print(hVoltage);
		Serial.print("|");
		Serial.print(sVoltage);
		Serial.print("\n");


		//Display Data
		switch (page)
		{
			case 1:
			lcd.setCursor(0, 0);
			lcd.print("House Bank V:");
			lcd.setCursor(14, 0);
			lcd.print(hVoltage);
			lcd.setCursor(0, 1);
			lcd.print("House Bank A:");
			lcd.setCursor(14, 1);
			lcd.print(hAmp);
			lcd.setCursor(0, 2);
			lcd.print("House Bank %:");
			lcd.setCursor(14, 2);
  			if (hVoltage < 13.0){         
                          lcd.print(hVoltagePercent);
                        }
                        else {
                          lcd.print("Charg");
                          }
			lcd.setCursor(0, 3);
			lcd.print("Start Bank V:");
			lcd.setCursor(14, 3);
			lcd.print(sVoltage);
                        //lcd.print(hCapacity / pow(hAmp, pExp));
			break;
			case 2:
			lcd.setCursor(0, 0);
			lcd.print("Engine RPM:");
			lcd.setCursor(14, 0);
			lcd.print(rpm);
			lcd.setCursor(0, 1);
			lcd.print("Engine Hours:");
			lcd.setCursor(14, 1);
			lcd.print(engHours);
			lcd.setCursor(0, 2);
			lcd.print("Fuel:");
			lcd.setCursor(14, 2);
			lcd.print(fuel);
			lcd.setCursor(0, 3);
			lcd.print("Average GPH:");
			lcd.setCursor(14, 3);
			lcd.print(avgGPH);
			break;
			case 3:
			lcd.setCursor(0, 0);
			lcd.print("Cabin Temp:");
			lcd.setCursor(14, 0);
			lcd.print(cabinTemp);
			lcd.setCursor(0, 1);
			lcd.print("Humidty:");
			lcd.setCursor(14, 1);
			lcd.print(cabinHumid);
			lcd.setCursor(0, 2);
			lcd.print("Bilge Cycles:");
			lcd.setCursor(14, 2);
			lcd.print(bilgeCycles);
			lcd.setCursor(0, 3);
			lcd.print("Alarms:");
			lcd.setCursor(14, 3);
			lcd.print(alarms);
			break;

		}
	}
}

