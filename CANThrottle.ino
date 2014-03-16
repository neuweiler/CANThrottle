/*
 * CAN Throttle
 *
 * Author: Michael Neuweiler
 * Created: 08.06.2013
 *
 * A small Arduino Due program which sends a CAN frame to
 * a Volvo S80 ECU to query the throttle angle.
 * It outputs the response (0x00 - 0xFF) as percentage to
 * an LCD shield.
 *
 * WARNING: This works on a Volvo S80 2008 model. You must
 * first find out the correct frame to query the throttle
 * angle and also the correct ID's. If you just test this
 * with your car prior to adjusting the CAN speed and frame
 * data, you might get into trouble. I take no responsibilty
 * whatsoever if you damage your car. You must know what
 * you are doing!
 *
 * A library for the Due's CAN bus is available here:
 * https://github.com/collin80/due_can
 *
 * For a design of a dual CAN shield, check my blog
 * http://s80ev.blogspot.ch/2013/05/springtime.html
 * or http://arduino.cc/forum/index.php/topic,131096.30.html
 *
 */

#include "CANThrottle.h"

#define CFG_LCD_MONITOR_PINS 8, 9, 4, 5, 6, 7 // specify the pin sequence used for LiquidCrystal initialisation
#define CFG_LCD_MONITOR_COLUMNS 16 // specify the number of columns of the display
#define CFG_LCD_MONITOR_ROWS 2 // specify the number of rows of the display

#define CAN_THROTTLE_REQUEST_DELAY 200 // milliseconds to wait between sending throttle requests

#define VOLVO_V50
//#define VOLVO_S80

#ifdef VOLVO_V50
#define CAN_THROTTLE_REQUEST_ID 0x3FFFE
#define CAN_THROTTLE_REQUEST_DATA {0xcd, 0x11, 0xa6, 0x00, 0x24, 0x01, 0x00, 0x00}
#define CAN_THROTTLE_REQUEST_EXT true
#define CAN_THROTTLE_RESPONSE_ID 0x21
#define CAN_THROTTLE_RESPONSE_MASK 0x1fffffff
#define CAN_THROTTLE_RESPONSE_EXT true
#define CAN_THROTTLE_DATA_BYTE 6
#endif // VOLVO_V50

#ifdef VOLVO_S80
#define CAN_THROTTLE_REQUEST_ID 0x7e0
#define CAN_THROTTLE_REQUEST_DATA {0x03, 0x22, 0xee, 0xcb, 0x00, 0x00, 0x00, 0x00}
#define CAN_THROTTLE_REQUEST_EXT false
#define CAN_THROTTLE_RESPONSE_ID 0x7e8
#define CAN_THROTTLE_RESPONSE_MAS 0x7ff
#define CAN_THROTTLE_RESPONSE_EXT false
#define CAN_THROTTLE_DATA_BYTE 4
#endif // VOLVO_S80

LiquidCrystal lcd(CFG_LCD_MONITOR_PINS);
uint32_t lastRequestTime;
byte data[] = CAN_THROTTLE_REQUEST_DATA;

void sendRequest();
void handleResponse(CAN_FRAME &);

void setup()
{
	Serial.begin(115200);
	Serial.println("CAN Throttle");

	lastRequestTime = 0;

	lcd.begin(CFG_LCD_MONITOR_COLUMNS, CFG_LCD_MONITOR_ROWS);
	lcd.setCursor(0, 0);
	lcd.print("CAN Throttle");

	// Initialize the CAN bus
	CAN2.init(CAN_BPS_500K);
	//Initialize mailbox 0 to receive messages from the ECU Id
	CAN2.setRXFilter(0, CAN_THROTTLE_RESPONSE_ID, CAN_THROTTLE_RESPONSE_MASK, CAN_THROTTLE_RESPONSE_EXT);

	Serial.println("setup complete");
}

void loop()
{
	// after the specified amount of milliseconds has passed, send out the request
	if ((millis() - lastRequestTime) > CAN_THROTTLE_REQUEST_DELAY) {
		sendRequest();
		lastRequestTime = millis();
	}
	// check if there is a response
	if (CAN2.rx_avail()) {
		CAN_FRAME incoming;
		CAN2.get_rx_buff(incoming);
		handleResponse(incoming);
	}
}

/*
 * Send a request to the ECU.
 *
 * V50:
 * Trace log of Vida: [0x00, 0xf, 0xff, 0xfe, 0xcd, 0x11, 0xa6, 0x00, 0x24, 0x01, 0x00, 0x00]
 * Trace log of CANMonitor: dlc=0x08 fid=0xFFFFE id=0x3FFFE ide=0x01 rtr=0x00 data=0xCD,0x11,0xA6,0x00,0x24,0x01,0x00,0x00,
 * S80:
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe0, 0x22, 0xee, 0xcb]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e0 id=0x7e0 ide=0x00 rtr=0x00 data=0x03,0x22,0xEE,0xCB,0x00,0x00,0x00,0x00
 *
 */
void sendRequest() {
	CAN_FRAME frame;

	Serial.print(millis());
	Serial.println(" - sending request");

	frame.length = 0x08;
	frame.priority = 10;
	frame.id = CAN_THROTTLE_REQUEST_ID;
	frame.extended = CAN_THROTTLE_REQUEST_EXT;
	memcpy(frame.data.bytes, data, 8);

	CAN2.sendFrame(frame);
}

/*
 * Handle the response of the ECU. Log the data on the serial port
 * and convert the response value to a percentage to display on
 * the LCD.
 *
 * V50:
 * Trace log of Vida: [0x00, 0x40, 0x00, 0x21, 0xce, 0x11, 0xe6, 0x00, 0x24, 0x03, 0xfd, 0x00] (2nd last byte contains throttle value)
 * Trace of CANMonitor: dlc=0x08 fid=0x400021 id=0x21 ide=0x01 rtr=0x00 data=0xCE,0x11,0xE6,0x00,0x24,0x03,0xFD,0x00
 * S80:
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00
 *
 * Note how vida represents the ID as part of the data (0x7e8 -->
 * 0x07, 0xe8) and how it ommits the first data byte which represents
 * the number of remaining bytes in the frame.
 */
void handleResponse(CAN_FRAME &frame) {
	Serial.print(micros());
	Serial.print(" - CAN: dlc=0x");
	Serial.print(frame.length, HEX);
	Serial.print(" fid=0x");
	Serial.print(frame.fid, HEX);
	Serial.print(" id=0x");
	Serial.print(frame.id, HEX);
	Serial.print(" ide=0x");
	Serial.print(frame.extended, HEX);
	Serial.print(" rtr=0x");
	Serial.print(frame.rtr, HEX);
	Serial.print(" data=");
	for (int i = 0; i < 8; i++) {
		Serial.print("0x");
		Serial.print(frame.data.bytes[i], HEX);
		Serial.print(",");
	}
	Serial.println();

	if (frame.id == CAN_THROTTLE_RESPONSE_ID) {
		lcd.setCursor(0, 0);
		lcd.print("CAN Throttle: ");
		lcd.setCursor(0, 1);
		lcd.print((float) frame.data.bytes[CAN_THROTTLE_DATA_BYTE] * 100.0 / 255.0);
		lcd.print("% ");
	}
}

