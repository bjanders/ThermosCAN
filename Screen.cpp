// 
// 
// 

#include <Adafruit_SSD1351.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <OneWireExt.h>
#include "Screen.h"
#include "ThermosCAN.h"

extern Adafruit_SSD1351 display;
extern Config config;
extern short owTemps[MAX_OW_DEVICES];

extern OneWire oneWire;
extern byte owDeviceCount;
extern byte mainOwTempIndex;
extern byte owDevices[MAX_OW_DEVICES][OW_ADDRESS_LEN];

DefaultScreen defaultScreen = DefaultScreen();
SetTempScreen setTempScreen = SetTempScreen();
ConfigureScreen configureScreen = ConfigureScreen();
SetDeviceIDScreen setDeviceIDScreen = SetDeviceIDScreen();

char lineBuffer[10]; // General purpose buffer for formatting text to displayed on the screeen

void saveConfig() {
	EEPROM.put(0, config);
}



void Screen::onEnter()
{
}

void Screen::onExit()
{
}

Screen *Screen::onLongClick()
{
	return this;
}

Screen *Screen::onShortClick()
{
	return this;
}

Screen *Screen::onLeftRight(int i)
{
	return this;
}

Screen *Screen::onUpDown(int i)
{
	return this;
}

Screen *Screen::onLoop(unsigned long millis)
{
	return this;
}

void DefaultScreen::onEnter()
{
	display.fillScreen(BLACK);
	//display.fillRect(0, 50, 180, 24, BLACK);
	display.setTextColor(WHITE);
	tempDisplayed = owTemps[mainOwTempIndex];
	owTempToStr(lineBuffer, tempDisplayed);
	refreshTemp();
	//display.fillRect(0, 80, 180, 16, BLACK);
	display.setCursor(40, 80);
	display.setTextSize(2);
	owTempToStr(lineBuffer, config.setTemp);
	display.print(lineBuffer);
	display.write(DEG);
}

Screen *DefaultScreen::onLoop(unsigned long millis)
{

	if (owTemps[mainOwTempIndex] != tempDisplayed) {
		tempDisplayed = owTemps[mainOwTempIndex];
		owTempToStr(lineBuffer, tempDisplayed);
		refreshTemp();
	}
	return this;
}

Screen *DefaultScreen::onShortClick()
{
	return &setTempScreen;
}

Screen *DefaultScreen::onLongClick()
{
	return &configureScreen;
}

void DefaultScreen::refreshTemp()
{
	display.setTextSize(3);
	display.fillRect(20, 50, 180, 24, BLACK);
	display.setCursor(20, 50);
	display.print(lineBuffer);
	display.write(DEG);
}


//void DefaultScreen::readTemp(unsigned long millisNow)
//{
//	// Send temperature convert to all devices
//	if (oneWire.reset() == 0) {
//		// No devices found
//		Serial.println("No devices found");
//		return;
//	}
//	oneWire.skip();
//	oneWire.write(OW_TEMP_CONVERT);
//	while (oneWire.read_bit() == 0) {};
//	// busy loop until convertion is done
//	// FIX: How long do we expect it to take?
//	// FIX: Don't wait forever
//	// We could also request a convert and then come back in a second or so
//	// to read the scratchpad
//	for (int i = 0; i < owDeviceCount; i++) {
//		oneWire.reset();
//		oneWire.select(owDevices[i]);
//		oneWire.write(OW_TEMP_READ_SCRATCH);
//		oneWire.read_bytes((byte *)&owTemps[i], 2);
//		//		formatTemp(lineBuffer, owTemps[i]);
//	}
//	millisTempUpdated = millisNow;
//}

void SetTempScreen::onEnter()
{
	refreshScreen();
}

void SetTempScreen::refreshScreen()
{
	display.fillRect(0, 80, SCREEN_WIDTH, ROW_HEIGHT * 2, RED);
	display.setTextColor(WHITE);
	display.setCursor(40, 80);
	display.setTextSize(2);
	owTempToStr(lineBuffer, config.setTemp);
	display.print(lineBuffer);
	display.write(DEG);
}

Screen *SetTempScreen::onUpDown(int upDown) {
	if (config.setTemp + upDown > 10 << 4 && config.setTemp + upDown < 25 << 4) {
		config.setTemp += upDown;
		//Serial.println(config.targetTemp & 0xf, HEX);
		// Skip temps with resolution below 0.1
		switch (config.setTemp & 0xf) {
		case 0x1:
		case 0x3:
		case 0x6:
		case 0x9:
		case 0xB:
		case 0xE:
			if (upDown > 0) {
				//Serial.println('+');
				config.setTemp++;
			} else {
				//Serial.println('-');
				config.setTemp--;
			}
		}
		refreshScreen();
	}
	return this;
}

Screen *SetTempScreen::onShortClick()
{
	saveConfig();
	return &defaultScreen;
}

void ConfigureScreen::onEnter()
{
	selectedItem = 0;
	refreshScreen();
}

void ConfigureScreen::refreshScreen()
{
	display.fillScreen(BLACK);
	display.setTextSize(MENU_TEXT_SIZE);
	display.setCursor(0, 0);
	display.fillRect(0, selectedItem * ROW_HEIGHT * MENU_TEXT_SIZE, SCREEN_WIDTH, ROW_HEIGHT * MENU_TEXT_SIZE, RED);
	display.print("Target temp:" );
	sprintf(lineBuffer, "%03X", config.setTempCANId);
	display.println(lineBuffer);
	for (int i = 0; i < owDeviceCount; i++) {
		owAddressToStr(lineBuffer, owDevices[i]);
		display.print(lineBuffer);
		display.print(": ");
		if (memcmp(owDevices[i], config.mainOwAddress, 8) == 0) {
			// Mark the main temp sensor
			sprintf(lineBuffer, "%03X *", config.owCANId[i]);
		} else {
			sprintf(lineBuffer, "%03X", config.owCANId[i]);
		}
		display.println(lineBuffer);
	}
	display.println("Exit");
}

Screen *ConfigureScreen::onUpDown(int upDown) {
	if (selectedItem - upDown >= 0 && selectedItem - upDown < owDeviceCount + 2) {
		selectedItem -= upDown;
		refreshScreen();
	}
	return this;
}

Screen *ConfigureScreen::onShortClick() {
	if (selectedItem == 0) {
		setDeviceIDScreen.setInfo("Target temp", &config.setTempCANId);
		return &setDeviceIDScreen;
	}
	// Exit
	if (selectedItem == owDeviceCount + 1) {
		return &defaultScreen;
	}
	owAddressToStr(lineBuffer, owDevices[selectedItem - 1]);
	setDeviceIDScreen.setInfo(lineBuffer, &config.owCANId[selectedItem - 1]);
	return &setDeviceIDScreen;
	//return menu[selectedItem].screen;
}

Screen *ConfigureScreen::onLongClick() {
	return &defaultScreen;
}

void SetDeviceIDScreen::onEnter() {
	nibble = 2;
	leftRight = 0;
	refreshScreen();
}

Screen *SetDeviceIDScreen::onShortClick()
{
	if (nibble - 1 >= 0) {
		nibble--;
		refreshScreen();
	} else if (nibble == 0) {
		*configId = workId;
		saveConfig();
		return &configureScreen;
	}
	return this;
}

Screen *SetDeviceIDScreen::onLongClick()
{
	return &configureScreen;
}

Screen *SetDeviceIDScreen::onUpDown(int upDown) {
	Serial.println(workId >> (nibble * 4) & 0xf, HEX);
	if (upDown > 0 && (workId >> (nibble * 4) & 0xf) == 0xf) {
		Serial.println("+");
		return this;
	}
	if (upDown < 0 && (workId >> (nibble * 4) & 0xf) == 0x0) {
		Serial.println("-");
		return this;
	}
	if (workId + (upDown << (nibble * 4)) >= 0 && workId + (upDown << (nibble * 4)) <= 0xeff) {
		workId += (upDown << (nibble * 4));
		refreshScreen();
	}
	return this;
}

Screen *SetDeviceIDScreen::onLeftRight(int leftRight)
{
	this->leftRight += leftRight;
	if (this->leftRight > 3) {
		leftRight = 1;
		this->leftRight = 0;
	} else if (this->leftRight < -3) {
		leftRight = -1;
		this->leftRight = 0;
	} else {
		return this;
	}
	if (nibble + leftRight >= 0 && nibble + leftRight <= 2) {
		nibble += leftRight;
		refreshScreen();
	}
	return this;
}


void SetDeviceIDScreen::setInfo(char *text, unsigned short *id) {
	strncpy(this->text, text, TEXT_LEN);
	configId = id;
	workId = *id;
}

void SetDeviceIDScreen::refreshScreen()
{
	display.fillScreen(BLACK);
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println("Set CAN ID for");
	display.println(text);
	display.setTextSize(3);
	display.setCursor(40, 40);
	display.fillRect(21 + (3 - nibble) * 18, 63, 16, 3, RED);
	sprintf(lineBuffer, "%03X", workId);
	display.print(lineBuffer);
}