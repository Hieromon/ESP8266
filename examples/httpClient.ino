#include "Arduino.h"
#include "ESP8266.h"
// If SOFTWARSERIAL or DebugSerial is applied,
// requires declaration of SortwareSerial.h
#include "SoftwareSerial.h"
SoftwareSerial	DebugSerial(8, 9);

#define SSID	"WARPSTAR-9FD487-G"
#define PWD		"BC5457B5C4E0A"

void setup() {
	char	*ipAddress, ap[31];

	DebugSerial.begin(9600);
	DebugSerial.println(F("Setup"));
	if (WiFi.reset(WIFI_RESET_HARD))
		DebugSerial.println(F("reset Complete"));
	else {
		DebugSerial.println(F("reset fail"));
		while (1);
	}
	if (WiFi.begin(9600)) {
		if (WiFi.setup(WIFI_CONN_CLIENT, WIFI_PRO_TCP, WIFI_MUX_SINGLE) != WIFI_ERR_OK) {
			DebugSerial.println(F("setup fail"));
			while (1);
		}
	} else {
		DebugSerial.println(F("begin fail"));
		while (1);
	}
	if (WiFi.join(SSID, PWD) == WIFI_ERR_OK) {
		ipAddress = WiFi.ip(WIFI_MODE_STA);
		DebugSerial.print(F("\n\rIP address:"));
		DebugSerial.print(ipAddress);
		DebugSerial.print(':');
		if (WiFi.isConnect(ap))
			DebugSerial.println(ap);
		else
			DebugSerial.println(F("not found"));
	} else {
		DebugSerial.println(F("connect fail"));
		while (1);
	}
}

void loop() {
	if (WiFi.connect((char *)"www.google.co.jp", 80) == WIFI_ERR_CONNECT) {
		DebugSerial.println(F("Send Start"));
		if (WiFi.send((const uint8_t *)"GET / HTTP/1.1\r\n\r\n") == WIFI_ERR_OK) {
			int16_t	c;
			uint16_t len = WiFi.listen(10000UL);
			DebugSerial.print(F("\nRCV:"));
			DebugSerial.print(len);
			DebugSerial.print(F("-->"));
			while (len)
				if ((c = WiFi.read()) > 0) {
					DebugSerial.write((char)c);
					len--;
				}
			DebugSerial.println(F("<--RCV"));
		} else {
			DebugSerial.println(F("Send fail"));
		}
		WiFi.close();
	} else {
		DebugSerial.println(F("TCP connect fail"));
	}
	WiFi.disconnect();
	while (1);
}
