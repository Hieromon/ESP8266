ESP8266 WiFi Library for Arduino
---
ESP8266 library for Arduino that facilitates the implementation of WiFi communication via user sketches.

[ESP8266] is a very powerful and low cost WiFi module. It is contained with a sufficient size of EEPROM and a 32-bit MPU necessary to  TCP/IP protocol stack built-in. You can easily build a WiFi device with a serial communication from physical computing boards such as Arduino and Raspberry Pi.

[ESP8266]:https://nurdspace.nl/ESP8266 "ESP8266 wiki"

ESP8266 WiFi Library for Arduino provides a function for easily WiFi communication using ESP8266 from your sketch via the serial on such as Arduino UNO and Arduino MEGA.

Also this library has a debug output facility can monitor the transmitted and received data.


Expamle the sketch
---
This exmaple sketch sends HTTP1.1 request to url of `www.google.co.jp` via TCP port 80 and it receives a response.<br>
The first step is join to a WiFi access point, and establish TCP connection with HTTP server. After that, send HTTP request and receive the response from the server. The series of steps will be used ESP8266 class methods.

```Arduino
#include "Arduino.h"
#include "ESP8266.h"

#include "SoftwareSerial.h"
SoftwareSerial	ConsoleOut(8, 9);

#define SSID	"MySSID"
#define PWD		"MyPassword"

void setup() {
	char	*ipAddress, ap[31];

	WiFi.reset(WIFI_RESET_HARD);
	WiFi.begin(9600);
	if (WiFi.join(SSID, PWD) == WIFI_ERR_OK) {
		ipAddress = WiFi.ip(WIFI_MODE_STA);
		ConsoleOut.print(F("\n\rIP address:"));
		ConsoleOut.print(ipAddress);
		ConsoleOut.print(':');
		if (WiFi.isConnect(ap))
			ConsoleOut.println(ap);
		else
			ConsoleOut.println(F("not found"));
	} else
		while (1);
}

void loop() {
	if (WiFi.connect((char *)"www.google.co.jp", 80) == WIFI_ERR_CONNECT) {

		if (WiFi.send((const uint8_t *)"GET / HTTP/1.1\r\n\r\n") == WIFI_ERR_OK) {
			int16_t	c;
			uint16_t len = WiFi.listen(10000UL);
			while (len)
				if ((c = WiFi.read()) > 0) {
					ConsoleOut.write((char)c);
					len--;
				}
		} else
			ConsoleOut.println(F("Send fail"));

		WiFi.close();

	} else
		ConsoleOut.println(F("TCP connect fail"));

	WiFi.disconnect();

	while (1);
}
```

Installation on your arduino IDE
---
Download a zip file of the ESP8266 library code from [Download ZIP](https://github.com/Hieromon/ESP8266/archive/master.zip "ESP8266 download a .zip") and you can import a .zip library in accordance with [here methods](http://www.arduino.cc/en/Guide/Libraries#toc4 "Importing a .zip Library") to your arduino IDE.

##ESP8266 Class##
