#include <EEPROM.h>
#include <SPI.h>
#include <mcp_can.h>
#include <OneWire.h> 
#include <OneWireExt.h>
#include <stdio.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include "ThermosCAN.h"
#include "Screen.h"

const unsigned short DEFAULT_TARGET_TEMP_CAN_ID = 0x7FF;
const unsigned short DEFAULT_OW_CAN_ID = 0x7FE;

short owTemps[MAX_OW_DEVICES];
byte mainOwTempIndex = 0;

//  PINS

const int ONE_WIRE_PIN = 12;
// SPI
const int CAN_CS_PIN = A4;
const int DISPLAY_DC_PIN = A2;
const int DISPLAY_CS_PIN = A1;
const int DISPLAY_RST_PIN = A3;
// Trackball
const int UP_PIN = 2;
const int DOWN_PIN = 3;
const int LEFT_PIN = 1;
const int RIGHT_PIN = 0;
const int BTN_PIN = 7;
const int RED_PIN = 10;

OneWire oneWire(ONE_WIRE_PIN);
byte owDevices[MAX_OW_DEVICES][OW_ADDRESS_LEN];
byte owDeviceCount = 0;

Config config;

const unsigned long SHORT_PRESS_MILLIS = 500;

Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN);
MCP_CAN CAN(CAN_CS_PIN);

volatile bool dirty = false;

unsigned long millisTempUpdated = 0;
const int TEMP_UPDATE_INTERVAL = 10000; // in milliseconds
unsigned long millisLedUpdated = 0;
const int BUTTON_LED_INTERVAL = 5;
byte led_pwm = 0;
 
volatile int upDown = 0;
volatile int leftRight = 0;
unsigned long millisButtonDown;

const unsigned short CONFIG_VERSION = 1;


volatile enum {
	NO_CLICK,
	SHORT_CLICK,
	LONG_CLICK
} buttonClick;

Screen *currentScreen = &defaultScreen;
Screen *nextScreen = currentScreen;
 
void setup() {
	Serial.begin(9600);
	display.begin();
	display.cp437(true);
	startupDisplay();
	pinMode(UP_PIN, INPUT);
	pinMode(DOWN_PIN, INPUT);
	pinMode(LEFT_PIN, INPUT);
	pinMode(RIGHT_PIN, INPUT);
	pinMode(BTN_PIN, INPUT);
	pinMode(RED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(UP_PIN), scrollUp, FALLING);
	attachInterrupt(digitalPinToInterrupt(DOWN_PIN), scrollDown, FALLING);
	attachInterrupt(digitalPinToInterrupt(LEFT_PIN), scrollLeft, FALLING);
	attachInterrupt(digitalPinToInterrupt(RIGHT_PIN), scrollRight, FALLING);
	attachInterrupt(digitalPinToInterrupt(BTN_PIN), pressButton, CHANGE);
	EEPROM.get(0, config);
	while (CAN_OK != CAN.begin(CAN_500KBPS, MCP_8MHz)) {
		delay(100);
	}
	searchDevices();
	if (owDeviceCount == 0) {
		Serial.println("No 1-wire devices found. Halting.");
		// FIX: show error message on display
		while (true) {};
	}
	bool configOk = false;
	if (config.version != CONFIG_VERSION) {
		configOk = false;
	} else {
		// if the version matches, check that the main 1-wire device exists
		for (int i = 0; i < owDeviceCount; i++) {
			if (memcmp(owDevices[i], config.mainOwAddress, 8) == 0) {
				// found it
				mainOwTempIndex = i;
				configOk = true;
				break; // break of of for loop
			}
		}
	}
	if (!configOk) {
		initEEPROM();
	}
	if (config.setTemp > 25 << 4) {
		config.setTemp = 21 << 4;
	}
	readTemp(millis());
	//display.fillScreen(BLACK);
	buttonClick = NO_CLICK;
	currentScreen->onEnter();
	Serial.println("Setup done");
}

void loop() {
	unsigned long millisNow = millis();
	//delay(20);
	if (buttonClick != NO_CLICK) {
		switch (buttonClick) {
		case SHORT_CLICK: nextScreen = currentScreen->onShortClick();
			break;
		case LONG_CLICK: nextScreen = currentScreen->onLongClick();
			break;
		}
		buttonClick = NO_CLICK;
	} else if (upDown != 0) {
		nextScreen = currentScreen->onUpDown(upDown);
		upDown = 0;
	} else if (leftRight != 0) {
		nextScreen = currentScreen->onLeftRight(leftRight);
		leftRight = 0;
	} else {
		nextScreen = currentScreen->onLoop(millisNow);
	}
	if (nextScreen != currentScreen) {
		currentScreen = nextScreen;
		currentScreen->onEnter();
	}
	if (millisNow - millisTempUpdated > TEMP_UPDATE_INTERVAL) {
		readTemp(millisNow);
		sendTemp();
	}
	delay(20);
}


void initEEPROM() {
	Serial.println("Initializing EEPROM");
	// The device is not configured
	// FIX: enter configuration screen, create default config for now
	// Use the first device automatically. Works well if no external
	// devices are connected.
	config.version = CONFIG_VERSION;
	memcpy(config.mainOwAddress, owDevices[0], 8);
	mainOwTempIndex = 0;
	config.setTempCANId = DEFAULT_TARGET_TEMP_CAN_ID;
	for (int i = 0; i < MAX_OW_DEVICES; i++) {
		config.owCANId[i] = DEFAULT_OW_CAN_ID;
	}
	saveConfig();
}

void searchDevices() {
	oneWire.reset_search();
	owDeviceCount = 0;
	while (oneWire.search(owDevices[owDeviceCount])) {
		owDeviceCount++;
	}
}

void readTemp(unsigned long millisNow)
{
	// Send temperature convert to all devices
	if (oneWire.reset() == 0) {
		// No devices found
		Serial.println("No devices found");
		return;
	}
	oneWire.skip();
	oneWire.write(OW_TEMP_CONVERT);
	while (oneWire.read_bit() == 0) {};
	// busy loop until convertion is done
	// FIX: How long do we expect it to take?
	// FIX: Don't wait forever
	// We could also request a convert and then come back in a second or so
	// to read the scratchpad
	for (int i = 0; i < owDeviceCount; i++) {
		oneWire.reset();
		oneWire.select(owDevices[i]);
		oneWire.write(OW_TEMP_READ_SCRATCH);
		oneWire.read_bytes((byte *)&owTemps[i], 2);
	}
	millisTempUpdated = millisNow;
}

void sendTemp()
{ 
	CAN.sendMsgBuf(config.setTempCANId, 0, 2, (byte *)&config.setTemp);
	for (int i = 0; i < owDeviceCount; i++) {
		CAN.sendMsgBuf(config.owCANId[i], 0, 2, (byte *)&owTemps[i]);
	}
}

void startupDisplay() {
	setDisplayContrast(5);
	display.fillScreen(BLACK);
	//printChars();
	display.setCursor(0, 50);
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.print("Initializing...");
}

void setDisplayContrast(byte contrast) {
	display.sendCommand(SSD1351_CMD_CONTRASTMASTER, &contrast, 1);
}

void scrollUp() {
	upDown++;
}

void scrollDown() {
	upDown--;
}
void scrollLeft() {
	leftRight--;
}

void scrollRight() {
	leftRight++;
}

void pressButton() {
	if (digitalRead(BTN_PIN) == 0) {
		// Record time button was pressed
		millisButtonDown = millis();
	} else if (millis() - millisButtonDown > SHORT_PRESS_MILLIS) {
		buttonClick = LONG_CLICK;
	} else {
		buttonClick = SHORT_CLICK;
	}
}


//void testPrint() {
//	short testVal[] = { 0x07D0, 0x0550, 0x0191, 0x00a2, 0x0008, 0x0000, 0xfff8, 0xff5e, 0xfe6f, 0xfc90 };
//	for (int i = 0; i < 10; i++) {
//		printTemp(testVal[i]);
//	}
//}

