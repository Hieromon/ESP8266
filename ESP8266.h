/**
	ESP8266 WiFi-Serial bridge library for the arduino.
	Version 0.9
	This software is released under the MIT License (MIT).
	http://opensource.org/licenses/mit-license.php
	Copyright (c) 2015 hieromon@gmail.com

	This is the #include header to the access with ESP8266 module.
	It includes all function definitions and constant definitions
	required to drive the various methods for communicating with ESP8266.
	The ESP8266 class that is defined by this header file to use an
	arduino serial library. An application sketch can be selected
	either software or hardware	serial. When it uses software serial
	the NewSoftwareSerial should be used.
*/

#ifndef __ESP8266_H__
#define __ESP8266_H__

//	SoftwareSerial choices are conditioned by activity of
//	the ESP8266_USE_SOFTWARESERAIL macro.
//	But it can not be used at the same time with software serial for
//	ESP8266 communication when using the DebugSerial.
//#define ESP8266_USE_SOFTWARESERIAL

//	Whether to enable capturing the transmission and reception for debugging.
//	Enable the DebugSerial to uncomment the following.
//#define ESP8266_USE_DEBUGSERIAL


#include "Arduino.h"
#if defined(ESP8266_USE_SOFTWARESERIAL) || defined(ESP8266_USE_DEBUGSERIAL)
// Directives at using software serial
#include "SoftwareSerial.h"
#endif

#ifdef ESP8266_USE_SOFTWARESERIAL
// Software serial buffer size assignment
typedef	SoftwareSerial	_ESP8266_SERIAL_TYPE;
// In the case of 15200bps in communication speed at the boot of ESP8266,
// increase to 256byts the buffer size which is defined in SoftwareSerial.h.
//#undef	__SS_MAX_RX_BUFF
//#define __SS_MAX_RX_BUFF		256
// SoftwareSerial capture pin assignment
#define _ESP8266_ALT_RX			8
#define _ESP8266_ALT_TX			9
// Default baudrate for communication between ESP8266 and an arduino
// SoftwareSerial baudrate works fine up to 9600 as recommended at uno.
#define ESP8266_DEF_BAUDRATE	9600
#else

// Directives at using hardware serial
// Serial object assignment for the hardware access
typedef HardwareSerial	_ESP8266_SERIAL_TYPE;
// _ESP8266_SERIAL macro defines the actual object of the serial class.
// If you want use other serial, the Serial would be changed along
// the implementation of the board. e.g. Serial1, Serial2, ...
#define _ESP8266_SERIAL			Serial
// Default baudrate for communication between ESP8266 and an arduino
#define ESP8266_DEF_BAUDRATE	115200
#endif

// Declarations for applying the DebugSerial
#ifdef ESP8266_USE_DEBUGSERIAL
#define _ESP8266_DBG_BAUDRATE	9600		// DebugSeral default baud rate
#define _ESP8266_DBG_RX			8			// Capture pin for receiving
#define _ESP8266_DBG_TX			9			// Capture pin for transmitting
extern	SoftwareSerial	DebugSerial;		// Export DebugSerail instance
#endif

// ES8266 reset configuration as follows.
// It provides a reset prescription by hardware or software for ESP8266.
// If the sketch uses hardware reset to ESP8266, Macro definition of
// ESP8266_RST_PIN must be assigned to the arduino pin for RST of ESP8266.
#define ESP8266_RST_PIN			2
// If the sketch does not use hardware reset, ESP8266_RST_PIN should not be
// defined. AT+RST command is used for an alternative to the ESP8266 by
// software reset.
typedef enum {
	WIFI_RESET_HARD,						// Use RST pin
	WIFI_RESET_SOFT							// Use AT+RST command
} WIFI_RESET;

// Declaration of constants
// Time-out limit for the waiting reply from ESP8266
#define ESP8266_DEF_TIMEOUT		3000
// Enumerator for the AT command to drive the ESP8266
// Error condition identifiers
typedef enum {
	WIFI_ERR_TIMEOUT = -1,					// Time-out occurred at listen from serial
	WIFI_ERR_OK = 0,						// Command successful
	WIFI_ERR_ERROR = 1,						// Command error
	WIFI_ERR_CONNECT,						// IP connection has been completed
	WIFI_ERR_SENDOK,						// Send successful
	WIFI_ERR_BUSY,							// Transmission busy
	WIFI_ERR_SENDFAIL,						// Sending failed
	WIFI_ERR_CLOSED							// IP connection has been closed
} WIFI_ERR;
// An activity to intended of the ESP8266 operation
typedef enum {								// WiFi mode for AT+CWMODE
	WIFI_MODE_STA = 1,						// Station
	WIFI_MODE_AP = 2,						// Access point
	WIFI_MODE_APSTA = 3						// Both
} WIFI_MODE;
// Multiple connection establishment
typedef enum {								// Multi-connection specification for AT+CIPMUX
	WIFI_MUX_SINGLE = 0,					// Single connection
	WIFI_MUX_MULTI = 1						// Multi connection
} WIFI_MUX;
// 
typedef enum {								// Transfer mode for AT+CIPMODE
	WIFI_IPMODE_NODESC = -1,				// Not specified
	WIFI_IPMODE_NORMAL = 0,					// Normal
	WIFI_IPMODE_BARE = 1					// Unvarnished transmission mode
} WIFI_IPMODE;
// Using protocol
typedef enum {								// Protocol specification for AT+CIPSTART
	WIFI_PRO_TCP,							// Use TCP
	WIFI_PRO_UDP							// Use UDP
} WIFI_PRO;

// Operation conditions and status values
// Active connection or passive connection
typedef enum {								// The application sketch connection type
	WIFI_CONN_NONE,							// None
	WIFI_CONN_SERVER,						// Passive connection, use AT+CIPSERVER
	WIFI_CONN_CLIENT,						// Active connection, use AT+CIPSTART
	WIFI_CONN_PEER							// Both
} WIFI_CONN;
// IP connection status
typedef enum {								// Return enumeration from AT+CIPSTATUS
	WIFI_STATUS_GOTIP,
	WIFI_STATUS_CONN,
	WIFI_STATUS_DISCONN,
	WIFI_STATUS_NOTCONN,
	WIFI_STATUS_UNKNOWN
} WIFI_STATUS;

// It presents the AT version of ESP8266 firmware
#define ESP8266_AT_VERSION	022

// ESP8266 class declaration
class ESP8266 {

private:
	// Private members
	_ESP8266_SERIAL_TYPE	*_uart;			// A class instance for serial access
	uint32_t	_baudrate;					// ESP8266 access baudrate
	WIFI_CONN	_conn;						// Connection topology
	WIFI_PRO	_protocol;					// Applied protocol for the current connection
	char		_ipAddrSta[16];				// Station IP address for this ESP8266
	char		_ipAddrAp[16];				// Access point IP address for this ESP8266

	// Private methods
	WIFI_ERR	_connect(int8_t channel, WIFI_PRO protocol, char *address, uint16_t port);
	WIFI_ERR	_send(int8_t channel, const uint8_t *data);
	void		setBaudrate(uint32_t baudrate);
	void		readFlush(void);
	WIFI_ERR	response(uint32_t timeout = ESP8266_DEF_TIMEOUT);
	bool		scan(const char *token);
	int8_t		readUntil(uint8_t *result, uint8_t terminator);

public:
	// Constructor
	ESP8266(_ESP8266_SERIAL_TYPE &uart);
	// Hardware reset for HSP8266 module.
	bool		reset(WIFI_RESET rst);
	// Begin WIFI connection and transmission.
	bool		begin(uint32_t baudrate = ESP8266_DEF_BAUDRATE);
	// End WIFI connection.
	void		end(void);
	// Configure connection mode and multi connection.
	WIFI_ERR	config(WIFI_MODE mode, WIFI_MUX mux, WIFI_IPMODE ipMode);
	// Connect to the WiFi access point for the station.
	WIFI_ERR	join(const char *ssid, const char *pwd);
	// Disconnect from the WiFi access point.
    WIFI_ERR	disconnect(void);
	// Inquire the connection establishment status with specified the access point.
	bool		isConnect(char *ssid);
	// Get IP address and report resulted IP address string.
	char		*ip(WIFI_MODE mode);
	// Inquire the current WiFi connection status.
	WIFI_STATUS	status(void);
	// Setup access connection topology.
	WIFI_ERR	setup(WIFI_CONN conn, WIFI_PRO protocol, WIFI_MUX mux = WIFI_MUX_MULTI);
	// Start the single IP connection for client side with active OPEN.
	WIFI_ERR	connect(char *address, uint16_t port);
	// Start IP connection with connection ID for multi connection.
	WIFI_ERR	connect(int8_t channel, char *address, uint16_t port);
	// Start IP connection for server side with passive SYN.
	WIFI_ERR	server(uint16_t port);
	// Sending data along with making a connection establishment.
	WIFI_ERR	send(int8_t channel, char *address, uint16_t port, const uint8_t *data);
	// Send data with connection ID specified.
	WIFI_ERR	send(int8_t channel, const uint8_t *data);
	// Send data with no connection ID specified.
	WIFI_ERR	send(const uint8_t *data);
	// Start listening, and then stores the received data to the buffer.
	int16_t		receive(uint8_t *buffer, uint16_t size, uint32_t timeOut = ESP8266_DEF_TIMEOUT);
	// Start listening at the specified connection, and then stores the received data to the buffer.
	int16_t		receive(int8_t channel, uint8_t *buffer, uint16_t size, uint32_t timeOut = ESP8266_DEF_TIMEOUT);
	// Starts the listening, and returns data length necessary for receiving.
	int16_t		listen(uint32_t timeOut = 0);
	// Starts the listening from selected connection, and returns data length necessary for receiving.
	int16_t		listen(int8_t channel, uint32_t timeOut = 0);
	// Get the number of bytes available for reading from ESP8266. 
	int16_t		available(void);
	// Return a character that was received from ESP8266.
	int16_t		read(void);
	// Close the currently active IP connection.
	void		close(void);
	// Close IP connection with specified connection ID.
	void		close(int8_t channel);
};

extern	ESP8266	WiFi;
#endif	/* __ESP8266_H__ */
