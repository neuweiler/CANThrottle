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

#define CAN_THROTTLE_REQUEST_ID 0x7e0  // the can bus id of the throttle level request
#define CAN_THROTTLE_RESPONSE_ID 0x7e8 // the can bus id of the throttle level response
#define CAN_THROTTLE_DATA_BYTE 4 // the number of the data byte containing the throttle level
#define CAN_THROTTLE_REQUEST_DELAY 200 // milliseconds to wait between sending throttle requests

LiquidCrystal lcd(CFG_LCD_MONITOR_PINS);
RX_CAN_FRAME incoming;
uint32_t lastRequestTime;

void sendRequest();
void handleResponse();

void setup()
{
	Serial.begin(115200);
	Serial.println("CAN Throttle");

	lastRequestTime = 0;

	lcd.begin(CFG_LCD_MONITOR_COLUMNS, CFG_LCD_MONITOR_ROWS);
	lcd.setCursor(0, 0);
	lcd.print("CAN Throttle");

	// Initialize the CAN bus
	CAN.init(SystemCoreClock, CAN_BPS_500K);

	//Initialize mailbox 0 to receive messages from the ECU Id
	CAN.mailbox_init(0);
	CAN.mailbox_set_mode(0, CAN_MB_RX_MODE);
	CAN.mailbox_set_accept_mask(0, CAN_THROTTLE_RESPONSE_ID, false);
	CAN.mailbox_set_id(0, CAN_THROTTLE_RESPONSE_ID, false);

	// Initialize mailbox 1 to send data to the ECU.
	CAN.mailbox_init(1);
	CAN.mailbox_set_mode(1, CAN_MB_TX_MODE);
	CAN.mailbox_set_priority(1, 10);
	CAN.mailbox_set_accept_mask(1, 0x7FF, false);

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
	if (CAN.mailbox_get_status(0) & CAN_MSR_MRDY) {
		handleResponse();
	}
}

/*
 * Send a request to the ECU.
 *
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe0, 0x22, 0xee, 0xcb]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e0 id=0x7e0 ide=0x00 rtr=0x00 data=0x03,0x22,0xEE,0xCB,0x00,0x00,0x00,0x00
 *
 */
void sendRequest() {

	Serial.print(millis());
	Serial.println(" - sending request");
	CAN.mailbox_set_id(1, CAN_THROTTLE_REQUEST_ID, false);
	CAN.mailbox_set_datalen(1, 8);
//TODO: convert the single databytes to a correct datal and datah value
//	CAN.mailbox_set_datal(1, 0x0322eecb);
//	CAN.mailbox_set_datah(1, 0x00000000);
	CAN.mailbox_set_databyte(1, 0, 0x03);
	CAN.mailbox_set_databyte(1, 1, 0x22);
	CAN.mailbox_set_databyte(1, 2, 0xee);
	CAN.mailbox_set_databyte(1, 3, 0xcb);
	CAN.mailbox_set_databyte(1, 4, 0x00);
	CAN.mailbox_set_databyte(1, 5, 0x00);
	CAN.mailbox_set_databyte(1, 6, 0x00);
	CAN.mailbox_set_databyte(1, 7, 0x00);

	CAN.global_send_transfer_cmd(CAN_TCR_MB1);
}

/*
 * Handle the response of the ECU. Log the data on the serial port
 * and convert the response value to a percentage to display on
 * the LCD.
 *
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00
 *
 * Note how vida represents the ID as part of the data (0x7e8 -->
 * 0x07, 0xe8) and how it ommits the first data byte which represents
 * the number of remaining bytes in the frame.
 */
void handleResponse() {
	CAN.mailbox_read(0, &incoming);

	Serial.print(micros());
	Serial.print(" - CAN: dlc=0x");
	Serial.print(incoming.dlc, HEX);
	Serial.print(" fid=0x");
	Serial.print(incoming.fid, HEX);
	Serial.print(" id=0x");
	Serial.print(incoming.id, HEX);
	Serial.print(" ide=0x");
	Serial.print(incoming.ide, HEX);
	Serial.print(" rtr=0x");
	Serial.print(incoming.rtr, HEX);
	Serial.print(" data=");
	for (int i = 0; i < 8; i++) {
		Serial.print("0x");
		Serial.print(incoming.data[i], HEX);
		Serial.print(",");
	}
	Serial.println();

	if (incoming.id == CAN_THROTTLE_RESPONSE_ID) {
		lcd.setCursor(0, 0);
		lcd.print("CAN Throttle: ");
		lcd.setCursor(0, 1);
		lcd.print((float) incoming.data[CAN_THROTTLE_DATA_BYTE] * 100.0 / 255.0);
		lcd.print("% ");
	}
}

