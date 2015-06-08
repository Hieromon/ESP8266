/**
	ESP8266 WiFi-Serial bridge library for the arduino.
	Version 0.9
	This software is released under the MIT License (MIT).
	http://opensource.org/licenses/mit-license.php
	Copyright (c) 2015 hieromon@gmail.com
  
	ESP8266 class implementation that contains whole members
	and methods. It includes the other instance assignment also.
*/

#include "ESP8266.h"

// To ensure the USE_DEBUGSERIAL to the DebugSerial enable.
#ifdef ESP8266_USE_DEBUGSERIAL
// Allocate SoftwareSerial instance for debug monitor
SoftwareSerial	DebugSerial(_ESP8266_DBG_RX, _ESP8266_DBG_TX);
// Allocate the scan monitoring buffer in the scan method
#define ESP8266_SCAN_BUFF_SIZE	32
static char	_DBG_SCAN_BUF[ESP8266_SCAN_BUFF_SIZE];
// Enable echo back of the command when it uses the DEBUGSERIAL.
#define ESP8266_AT_ATE		"ATE1"
// Common function of monitoring output for debugging
#define ESP8266_DebugWrite(x)	do { DebugSerial.write(x); } while(0)

#else
// DEBUGSERIAL is not available here.
// Unable the command echo back
#define ESP8266_AT_ATE		"ATE0"
// Debug write function is null
#define ESP8266_DebugWrite(x)	do {} while(0)
#endif

// Allocate actual object to communicate with the ESP8266
#ifdef ESP8266_USE_SOFTWARESERIAL
SoftwareSerial	_ESP8266_SERIAL(_ESP8266_ALT_RX, _ESP8266_ALT_TX);
#endif

// Default instance
// The instance as the WiFi would be exported to refer from the
// user sketch.
ESP8266	WiFi(_ESP8266_SERIAL);

// Resolve version differences of AT commands.
// Some AT commands are deprecated in older version and describe
// in the document of '4A-AT-Espressif AT Instruction Set_v0.22.pdf'
// for details.
#if ESP8266_AT_VERSION < 022
#define ESP8266_AT_UART		"AT+UART"
#define ESP8266_AT_CWMODE	"AT+CWMODE"
#define ESP8266_AT_CWJAP	"AT+CWJAP"
#else
#define ESP8266_AT_UART		"AT+UART_CUR"
#define ESP8266_AT_CWMODE	"AT+CWMODE_CUR"
#define ESP8266_AT_CWJAP	"AT+CWJAP_CUR"
#endif


/**
 * ESP8266 class constructor.
 * Choose to either use software serial or hardware serial to
 * ESP8266 access and specify baud rate for communication between
 * ESP8266 and an arduino.
 * @parameter	uart		The instance of Serial object
 * @parameter	baudrate	Baud rate
 */
ESP8266::ESP8266(_ESP8266_SERIAL_TYPE &uart) : _uart(&uart) {
	// Start constructor
	_baudrate = ESP8266_DEF_BAUDRATE;
	_conn = WIFI_CONN_NONE;

	// Start ESP8266 communication port.
	_uart->begin(_baudrate);
	_uart->setTimeout(ESP8266_DEF_TIMEOUT);

	// Configure for ESP8266 reset to apply the module restart.
#ifdef ESP8266_RST_PIN
	pinMode(ESP8266_RST_PIN, OUTPUT);
#endif

	// Prepare an alternative serial port for debugging.
#ifdef ESP8266_USE_DEBUGSERIAL
	DebugSerial.begin(_ESP8266_DBG_BAUDRATE);
#endif
}

/**
 * Reset ESP8266 by the RST signal with hardware.
 * @parameter	rst		WIFI_RESET enumeration value
 * @return		true	ESP3266 module successfully started
 *				false	some error occurred
 */
bool ESP8266::reset(WIFI_RESET rst) {
	switch (rst) {
	case WIFI_RESET_HARD:
		// Go on the reset sequence by hardware
		_uart->end();
		_uart->begin(ESP8266_DEF_BAUDRATE);
		_uart->setTimeout(ESP8266_DEF_TIMEOUT);
#ifdef ESP8266_RST_PIN
		digitalWrite(ESP8266_RST_PIN, LOW);
		delay(1);
		digitalWrite(ESP8266_RST_PIN, HIGH);
#endif
		break;
	case WIFI_RESET_SOFT:
		// Go on the reset sequence by AT+RST command
		_uart->println(F("AT+RST"));
		_uart->end();
		_uart->begin(ESP8266_DEF_BAUDRATE);
		_uart->setTimeout(ESP8266_DEF_TIMEOUT);
		delay(1000);
		break;
	}
	return scan("ready");
}

/**
 * Start UART session with ESP3266 communication.
 * @parameter	baudrate	Baud rate of UART for ESP32688 communication
 * @return		true		ESP3266 communication successfully started
 *				false		some error occurred
 */
bool ESP8266::begin(uint32_t baudrate) {
	//
	setBaudrate(baudrate);
	_uart->end();
	_uart->begin(baudrate);
	_uart->println(F(ESP8266_AT_ATE));
	if (response() == WIFI_ERR_OK) {
		readFlush();
		_baudrate = baudrate;
		return true;
	} else
		return false;
}

/**
 * Close UART session with ESP3266 communication.
 */
void ESP8266::end(void) {
	(void)disconnect();
	_uart->println(F("ATE1"));
	_uart->end();
}

/**
 * Configuration WiFi mode, IP transfer mode and multiple connection mode.
 * @parameter	mode	<code>WIFI_MODE</code> enumeration value for CWMODE
 * @parameter	mux		<code>WIFI_MUX</code> enumeration value for CIPMUX
 * @parameter	ipMode	<code>WIFI_IPMODE</code> enumeration value for CIPMODE
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::config(WIFI_MODE mode, WIFI_MUX mux, WIFI_IPMODE ipMode) {
	WIFI_ERR	err;

	// WIFI mode (station/softAP/station+softAP)
	_uart->print(F(ESP8266_AT_CWMODE "="));
	_uart->println((int)mode);
	if ((err = response()) != WIFI_ERR_OK)
		return err;
	// Enable multiple connections
	_uart->print(F("AT+CIPMUX="));
	_uart->println((int)mux);
	if ((err = response()) != WIFI_ERR_OK)
		return err;
	// Set transfer mode.
	// CIPMODE command is available if the IP is connected, so the
	// following step would be skipped when WIFI_IPMODE_NODESC is specified.
	if (ipMode != WIFI_IPMODE_NODESC) {
		_uart->print(F("AT+CIPMODE="));
		_uart->println((int)ipMode);
		err = response();
	}
	return err;
}

/**
 * Connect to the WiFi access point for the station.
 * @parameter	ssid	SSID of the access point to be connected
 * @parameter	pwd		Pass phrase
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::join(const char *ssid, const char *pwd) {
	_uart->print(F(ESP8266_AT_CWJAP "=\""));
	_uart->print(ssid);
	_uart->print(F("\",\""));
	_uart->print(pwd);
	_uart->println(F("\""));
	return response(10000);
}

/**
 * Get IP address and report resulted IP address string.
 * @parameter	mode	<code>WIFI_MODE</code> enumeration value
 *						should be announce
 * @return		IP address string
 */
char *ESP8266::ip(WIFI_MODE mode) {
	// Find IP address as the client
	_uart->println(F("AT+CIFSR"));
	if (scan("+CIFSR:STAIP,\""))
		(void)readUntil((uint8_t *)_ipAddrSta, (uint8_t)'"');
	// Find IP address as the SoftAP
	if (scan("+CIFSR:APIP,\""))
		(void)readUntil((uint8_t *)_ipAddrAp, (uint8_t)'"');
	else
		// +CIFSR is not response, Clear IP address
		// And it may not be the SoftAP mode 
		_ipAddrAp[0] = '\0';

	// Dispatching WiFi connection state to decide which the result.
	readFlush();
	switch (mode) {
	case WIFI_MODE_STA:
		return _ipAddrSta;
	case WIFI_MODE_AP:
		return _ipAddrAp;
	default:
		return NULL;
	}
}

/**
 * Disconnect from WiFi access point.
 * @return	WIFI_ERR
 */
WIFI_ERR ESP8266::disconnect(void) {
	_uart->println(F("AT+CWQAP"));
	return response();	
}

/**
 * Inquire current WiFi connection status.
 * @parameter	ssid	SSID of the WiFi AP
 * @return		True	Connected
 *				False	Not connected
 */
bool ESP8266::isConnect(char *ssid) {
	_uart->println(F("AT+CWJAP?"));
	if (scan("+CWJAP:\"")) {
		if (readUntil((uint8_t *)ssid, '"') > 0) {
			readFlush();
			return true;
		} else
			return false;
	} else
		return false;
}

/**
 * Get current WiFi connection status.
 * @return	WIFI_STATUS	The following values indicating the connection state.
 *		<code>WIFI_STATUS_GOTIP</code> IP address assigned
 *		<code>WIFI_STATUS_CONN</code> WiFi connected
 *		<code>WIFI_STATUS_DISCONN</code> WiFi disconnected
 *		<code>WIFI_STATUS_NOTCONN</code> WiFi did not connected
 */
WIFI_STATUS ESP8266::status(void) {
	WIFI_STATUS	sta = WIFI_STATUS_UNKNOWN;

	_uart->println(F("AT+CIPSTATUS"));
	if (scan("STATUS:"))
		switch (_uart->read()) {
		case '2' :
			sta = WIFI_STATUS_GOTIP;
			break;
		case '3' :
			sta = WIFI_STATUS_CONN;
			break;
		case '4' :
			sta = WIFI_STATUS_DISCONN;
			break;
		case '5' :
			sta = WIFI_STATUS_NOTCONN;
			break;
		}
	readFlush();
	return sta;
}

/**
 * Setup access connection topology.
 * Set the ESP8266 mode of making up the connection topology.
 * @parameter	conn	The following values indicating the connection topology.
 *	<code>WIFI_CONN_CLIENT</code> Client access
 *	<code>WIFI_CONN_SERVER</code> Server connection
 *	<code>WIFI_CONN_BROADCAST</code> Client access by UDP connection
 * @parameter	protocol	The following values indicating the connection type.
 *	<code>WIFI_PRO</code>	Transfer protocol to be used in this connection
 * @parameter	mux			The following values indicating the multiple connection.
 *	<code>WIFI_MUX</code>	Transfer protocol to be used in this connection
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::setup(WIFI_CONN conn, WIFI_PRO protocol, WIFI_MUX mux) {
	WIFI_ERR	err;

	switch (conn) {
	case WIFI_CONN_CLIENT:
		err = config(WIFI_MODE_STA, mux, WIFI_IPMODE_NODESC);
		break;
	case WIFI_CONN_SERVER:
		err = config(WIFI_MODE_AP, WIFI_MUX_MULTI, WIFI_IPMODE_NODESC);
		break;
	case WIFI_CONN_PEER:
		err = config(WIFI_MODE_APSTA, mux, WIFI_IPMODE_NODESC);
		break;
	default:
		err = WIFI_ERR_OK;
		break;
	}
	if (err == WIFI_ERR_OK)
		_protocol = protocol;
	return err;
}

/**
 * Start IP connection for server side with passive SYN.
 * Start the server side of the IP connection executed by CIPSERVER command.
 * @parameter	port		Port number
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::server(uint16_t port) {
	WIFI_ERR	err;

	_uart->print(F("AT+CIPSERVER=1,"));
	_uart->println(port);
	if ((err = response(10000)) == WIFI_ERR_CONNECT)
		_conn = WIFI_CONN_SERVER;
	readFlush();
	return err;
}
/**
 * Start the single IP connection for client side with active OPEN.
 * @parameter	address	IP address of the destination
 * @parameter	port	Port number as a connection
 */
WIFI_ERR ESP8266::connect(char *address, uint16_t port) {
	return _connect(-1, _protocol, address, port);
}
/**
 * Start IP connection with connection ID for multi connection.
 * @parameter	channel	Connection ID
 * @parameter	address	IP address of the destination
 * @parameter	port	Port number as a connection
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::connect(int8_t channel, char *address, uint16_t port) {
	return _connect(channel, _protocol, address, port);
}
/**
 * Start IP connection actual method.
 * @parameter	protocol	Connection protocol by <code>WIFI_PRO</code> enumeration value.
 * @parameter	channel		The connection id to be connected,
 *	If <code>-1</code> is specified then 0 would be assigned to the connection id.
 * @parameter	address		IP address to be connected.
 * @parameter	port		Port number.
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::_connect(int8_t channel, WIFI_PRO protocol, char *address, uint16_t port) {
	WIFI_ERR	err;

	// Start connection string building to order with ESP3266
	// by CIPSTART command.
	_uart->print(F("AT+CIPSTART="));
	if (channel >= 0) {
		_uart->print(channel);
		_uart->print(',');
	}
	// Dispatch protocol specification
	switch (protocol) {
	case WIFI_PRO_TCP:
		_uart->print(F("\"TCP\",\""));
		break;
	case WIFI_PRO_UDP:
		_uart->print(F("\"UDP\",\""));
		break;
	}
	// Append port number and throw to ESP8266
	_uart->print((char *)address);
	_uart->print(F("\","));
	_uart->println(port);
	// Waiting for reply
	if ((err = response(10000)) == WIFI_ERR_CONNECT)
		_conn = WIFI_CONN_CLIENT;
	// Flush remaining response string, end connecting
	readFlush();
	return err;
}

/**
 * Send data with no connection ID specified.
 * @parameter	data	sending data with null termination
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::send(const uint8_t *data) {
	return _send(-1, data);
}
/**
 * Send data with connection ID specified.
 * @parameter	channel	Connection ID to be used for the transmission
 * @parameter	data	sending data with null termination
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::send(int8_t channel, const uint8_t *data) {
	return _send(channel, data);
}
/**
 * Sending data along with making a connection establishment.
 * @parameter	channel	Connection ID to be used for the transmission
 * @parameter	address	IP address to be connected
 * @parameter	port	Port number
 * @parameter	data	sending data with null termination
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::send(int8_t channel, char *address, uint16_t port, const uint8_t *data) {
	WIFI_ERR	err;

	// Establish connection with the listener on UDP protocol
	if ((err = _connect(channel, _protocol, address, port)) != WIFI_ERR_CONNECT)
		return err;

	// Transmission data
	return _send(channel, data);
}
/**
 * Data sending actual method.
 * @parameter	channel	Connection ID to send
 * @parameter	buffer	Address that stores data for transmission
 * @return		WIFI_ERR
 */
WIFI_ERR ESP8266::_send(int8_t channel, const uint8_t *buffer) {
	WIFI_ERR	res;
	const uint8_t	*sp = buffer;
	int16_t	s_size = 0;

	// Determine sending length
	while (*sp++)
		s_size++;
	if (s_size <= 0 )
		return WIFI_ERR_ERROR;

	// Start forwarding
	_uart->print(F("AT+CIPSEND="));
	if (channel >= 0) {
		// Sending ID specified
		_uart->print(channel);
		_uart->print(',');
	}
	_uart->println(s_size);

	// Send request ended, 
	// Regard a acknowledgment as likely to send.
	if ((res = response()) == WIFI_ERR_OK) {
		if (scan("> ")) {
			while (*buffer)
				_uart->write(*buffer++);
			// Acknowledge the sending conclusion.
			// If NACK detected, reason identifying.
			res = response() == WIFI_ERR_SENDOK ? WIFI_ERR_OK : WIFI_ERR_ERROR;
		} else
			res = WIFI_ERR_TIMEOUT;
	}
	return res;
}

/**
 * Start listening at the specified connection, and then stores
 * the received data to the buffer. If the connection mode is
 * single, specify -1 to the channel argument.
 * @parameter	channel	connection ID
 * @parameter	buffer	Buffer to store the received data that
 *						header has been excluded '+PD:n'
 * @parameter	size	Buffer size
 * @parameter	timeOut	Time out scale, 0 without time-out
 * @return		Received data length
 */
int16_t ESP8266::receive(uint8_t *buffer, uint16_t size, uint32_t timeOut) {
	return receive(-1, buffer, size, timeOut);
}
int16_t ESP8266::receive(int8_t channel, uint8_t *buffer, uint16_t size, uint32_t timeOut) {
	uint32_t	startAt;
	int16_t		c;
	int16_t		wlen, rlen = 0;
	bool		cont;

	// Extract a length of receiving data.
	if ((wlen = listen(channel, timeOut)) > 0) {
		// Start the receiving by set the data length. 
		rlen = wlen;
		startAt = millis();
		do {
			if ((c = _uart->read()) >= 0) {
				*buffer++ = (uint8_t)c;
				ESP8266_DebugWrite((char)c);
				// reduce the length to be received, and then exit
				// after all data receiving.
				rlen--;
			}
			// Measure the occurrence of time-out.
			// A parameter as timeOut is 0, ignore time-out.
			cont = timeOut ? millis() - startAt < timeOut : true;
		} while (rlen && cont);
	}
	return wlen;
}

/**
 * Starts the listening from selected connection. If the connection
 * mode is single, specify -1 to the channel argument. When
 * the reception starts it will analyze the +IPAD identifier,
 * and returns data length necessary for receiving. Wait for
 * the reception indefinitely when give zero to timeOut argument.
 * If a timeout occurs return value will be zero.
 * @parameter	channel	connection ID
 * @parameter	timeOut Time out scale, 0 without time-out
 * @return		The length of the data to be received
 */
int16_t ESP8266::listen(uint32_t timeOut) {
	return listen(-1, timeOut);
}
int16_t ESP8266::listen(int8_t channel, uint32_t timeOut) {
	uint32_t	startAt;					// receiving start time for timeout measure
	int16_t		rlen = 0;
	int16_t		c;
	char		rcvCh[] = "+IPD,n,";
	bool		cont;						// ignore timeout

	// Create receiving channel identifier
	if (channel < 0)
		rcvCh[5] = '\0';
	else
		rcvCh[5] = (char)channel + '0';

	// To save the start time in order to measure the time-out.
	startAt = millis();
	do {
		if (_uart->available() > 0) {
			// Start '+IPD' identifier parsing
			// Extract the data length should be received.
			if (scan((const char *)rcvCh)) {
				do {
					while ((c = _uart->read()) < 0);
					// Debug write if available it.
					ESP8266_DebugWrite((char)c);
					// Lexical analysis of the presented data length.
					if ((char)c >= '0' && (char)c <= '9') {
						rlen *= 10;
						rlen += (int16_t)((char)c - '0');
					}
				} while ((char)c != ':');
				// Terminate the parsing when detect the lexical the
				// delimiter of data length.
				break;
			}
		}
		// timeOut argument zero to disable time-out.
		cont = timeOut ? (millis() - startAt < timeOut) : true;
	} while (cont);
	return rlen;
}

/**
 * Get the number of bytes available for reading from ESP8266.
 * This is data that's already arrived in the serial receive buffer.
 * @return		The number of bytes available to read
 */
int16_t ESP8266::available(void) {
	return _uart->available();
}

/**
 * Return a character that was received from ESP8266.
 * @return		The character read, or -1 if none is available
 */
int16_t ESP8266::read(void) {
	int16_t	c;
	c = _uart->read();
	ESP8266_DebugWrite((char)c);
	return c;
}

/**
 * Close the currently TCP/UPD active connection
 * Execute to either CIPCLOSE or CIPSERVER=0, it is determined by
 * the current connection type that is stored in <code>_conn</code>.
 * When <code>WIFI_CONN_SERVER</code> then CIPSERVER=0,
 * when <code>WIFI_CONN_CLIENT</code> then CIPCLOSE.
 * @parameter	channel	the connection id to be closed
 */
void ESP8266::close(void) {
	close(-1);
}
void ESP8266::close(int8_t channel) {
	readFlush();
	switch (_conn) {
	// Close the server connection
	case WIFI_CONN_SERVER:
		_uart->print(F("AT+CIPSERVER=0"));
		if (channel >= 0) {
			_uart->print(',');
			_uart->print(channel);
		}
		_uart->println();
		break;
	// Close the client connection
	case WIFI_CONN_CLIENT:
	case WIFI_CONN_PEER:
		_uart->print(F("AT+CIPCLOSE"));
		if (channel >= 0) {
			_uart->print('=');
			_uart->print(channel);
		}
		_uart->println();
		break;
	case WIFI_CONN_NONE:
		break;
	}
	_conn = WIFI_CONN_NONE;
	(void)response();
}

/**
 * Set current baudrate of UART temporary.
 * Baud rate of ESP8266 temporarily change the baudrate from 115200
 * so as to be capable of processing even slow CPU. Baud rate would
 * be returned to initial value by RST because it is changed by the
 * +UART_CUR command temporarily.
 * @parameter	baudrate	Baud rate to be set
  */
void ESP8266::setBaudrate(uint32_t baudrate) {
	_uart->print(F(ESP8266_AT_UART "="));
	_uart->print(baudrate);
	_uart->println(F(",8,1,0,0"));
	delay(100);
	readFlush();
}

/**
 * Empty the receive buffer by read out forcibly
 */
void ESP8266::readFlush(void) {
	int16_t		c;

	delay(3);
	while ((c = _uart->read()) >= 0) {
		ESP8266_DebugWrite((char)c);
		delay(3);
	}
}

// Search table to detect the discernable response from ESP8266 AT.
// This table is used by the response method as below and it has
// a state transition structure. Each term to be detected has a 
// state number, this number will identify the current matching
// position of each term in the receiving stream from ESP8266.
// When the received character will match to the term the state
// number should be increased. It will find the aimed response
// when the state number will reach to at end of the term.
struct {
	const char	*term;						// The term to be detected
	uint8_t		state;						// Byte offset which 
	WIFI_ERR	condition;					// Return condition when the term detected
} static _FIND_STATE[] = {
	{ "CONNECT\r\n", 0, WIFI_ERR_CONNECT },
	{ "SEND OK\r\n", 0, WIFI_ERR_SENDOK },
	{ "SEND FAIL",   0, WIFI_ERR_SENDFAIL },
	{ "CLOSED",		 0, WIFI_ERR_CLOSED },
	{ "busy",		 0, WIFI_ERR_BUSY },
	{ "\nERROR",	 0, WIFI_ERR_ERROR },
	{ "\nOK\r\n",	 0, WIFI_ERR_OK },
	{ NULL,			 0, WIFI_ERR_TIMEOUT }
};
/**
 * Waiting a response.
 * @parameter	timeOut		Time-out with millisecond unit
 * @return	The following values as WIFI_ERR enumeration indicating
 *	the error conditions.
 *	<code>WIFI_ERR_OK</code>		No error occurred
 *	<code>WIFI_ERR_ERROR</code>		some error occurred
 *	<code>WIFI_ERR_CONNECT</code>	IP connection establishment
 *	<code>WIFI_ERR_SEND FAIL</code>	Transmission failure occurrence
 *	<code>WIFI_ERR_CLOSED</code>	Connection closed
 *	<code>WIFI_ERR_BUSY</code>		Transmission busy
 */
WIFI_ERR ESP8266::response(uint32_t timeOut) {
	WIFI_ERR	err;
	uint32_t	start;
	int16_t		c;
	register uint8_t	iNode, state;

	// Initialize the state number that must be positioned at
	// start of the term.
	for (iNode = 0; _FIND_STATE[iNode].term != NULL; iNode++)
		_FIND_STATE[iNode].state = 0;

	// Save start time, start scan of receiving stream.
	err = WIFI_ERR_TIMEOUT;
	start = millis();
	do {
		// During the period of time following a state transition.
		if ((c = _uart->read()) >= 0) {
			ESP8266_DebugWrite((char)c);
			// Start comparison of phrase of the term with receiving stream.
			for (iNode = 0; _FIND_STATE[iNode].term != NULL; iNode++) {
				state = _FIND_STATE[iNode].state;
				// The state number reaches at end of the term, scan process
				// should be ended and the 'err' is set by the enumeration
				// value named as 'condition'.
				if ((char)c == _FIND_STATE[iNode].term[state]) {
					state++;
					if (_FIND_STATE[iNode].term[state] == '\0') {
						err = _FIND_STATE[iNode].condition;
						break;
					} else
						// If a receiving character matches the current
						// phrase of the term, state number would be increased.
						_FIND_STATE[iNode].state = state;
				}
			}
		}
	// At some term detection, escape from scanning.
	} while (err == WIFI_ERR_TIMEOUT && (millis() - start < timeOut));
	return err;
}

/**
 * Scan in the received data and determine the specified token has
 * arrived while receiving UART.
 * @parameter	token	Scanning token string
 * @return		true	It detected in the received data
 *				false	Not detected
 */
bool ESP8266::scan(const char *token) {
	uint32_t	start;
	int16_t		c;
	const char	*sp;
#ifdef ESP8266_USE_DEBUGSERIAL
	register uint8_t	rp = 0, rc = 0;
#endif

	sp = token;								// Save comparison source
	// Save starting time,
	// until even the longest reach in the time-out.
	start = millis();
	while (*sp) {
		if ((c = _uart->read()) >= 0) {
#ifdef ESP8266_USE_DEBUGSERIAL
			// Save a read character to the ring buffer.
			_DBG_SCAN_BUF[rp++] = (char)c;
			rp &= (ESP8266_SCAN_BUFF_SIZE - 1);
			if (++rc > ESP8266_SCAN_BUFF_SIZE)
				rc = ESP8266_SCAN_BUFF_SIZE;
#endif
			// Verify the read characters
			if ((char)c == *sp)
				sp++;
			else
				sp = token;
		}
		// Scan time-out
		if (millis() - start > ESP8266_DEF_TIMEOUT)
			break;
	}
#ifdef ESP8266_USE_DEBUGSERIAL
	while (rc--) {
		DebugSerial.write(_DBG_SCAN_BUF[rp++]);
		rp &= (ESP8266_SCAN_BUFF_SIZE - 1);
	}
#endif
	return (*sp == '\0');
}

/**
 * Receive until a specified character.
 * @parameter	result		Received characters storing buffer
 * @parameter	terminator	A character of termination
 * @return		Number of received characters
 */
int8_t ESP8266::readUntil(uint8_t *result, uint8_t terminator) {
	uint32_t	start;
	int16_t		c;
	register int8_t	count = 0;

	// Save starting time, Start scanning.
	start = millis();
	// Start scanning
	c = _uart->read();
	while ((uint8_t)c != terminator) {
		// Save available reading character
		if (c >= 0) {
			ESP8266_DebugWrite((char)c);
			// Stack a received character
			*result++ = (uint8_t)c;
			count++;
		}
		// until even the longest reach in the time-out.
		if (millis() - start > ESP8266_DEF_TIMEOUT)
			break;
		// Read next
		c = _uart->read();
	}
	*result = '\0';
	return count;
}
