// 
// 
// 

#include <Adafruit_SSD1351.h>
#include <OneWire.h> 
#include "Screen.h"


extern Adafruit_SSD1351 display;
extern int targetTemp; // target temperature multipled by 10
extern short owTemps[MAX_OW_DEVICES];

extern OneWire oneWire;
extern int owDeviceCount;
extern byte owDevices[MAX_OW_DEVICES][OW_ADDRESS_LEN];

DefaultScreen defaultScreen = DefaultScreen();
SetTempScreen setTempScreen = SetTempScreen();
ConfigureScreen configureScreen = ConfigureScreen();
char lineBuffer[10];


// target string should be at least 6 bytes
void formatTemp(char *s, short val) {
	char sign[2] = { 0, 0 };
	if (val < 0) {
		// Get two's complement and add the minus sign
		val = ~val + 1;
		sign[0] = '-';
	}
	// The decimal part is split up on 4 bits, i.e. 16 parts, 1000/16 = 62.5,
	// but 62 is sufficient for our needs and avoids floating point operations.
	// Add 50 to get proper rounding
	sprintf(s, "%s%d.%d", sign, val >> 4, ((val & 0xf) * 62 + 50) / 100);
	//	Serial.println(s);
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
	display.setTextSize(3);
	//display.setCursor(0, 0);
	//display.print("1234567890123456");
	display.setCursor(20, 50);
	display.print(lineBuffer);
	display.write(DEG);
	//display.fillRect(0, 80, 180, 16, BLACK);
	display.setCursor(40, 80);
	display.setTextSize(2);
	display.print(targetTemp / 10);
	display.print('.');
	display.print(targetTemp % 10);
	display.write(DEG);
}

Screen *DefaultScreen::onLoop(unsigned long millis)
{

	if (owTemps[0] != tempDisplayed) {
		tempDisplayed = owTemps[0];
		formatTemp(lineBuffer, tempDisplayed);
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
	display.print(targetTemp / 10);
	display.print('.');
	display.print(targetTemp % 10);
	display.write(DEG);
}

Screen *SetTempScreen::onUpDown(int upDown) {
	if (targetTemp + upDown > 100 && targetTemp + upDown < 250) {
		targetTemp += upDown;
		refreshScreen();
	}
	return this;
}

Screen *SetTempScreen::onShortClick()
{
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
	for (int i = 0; i < MENU_LENGTH; i++) {
		display.println(menu[i].text);
	}
}

Screen *ConfigureScreen::onUpDown(int upDown) {
	if (selectedItem + upDown > 0 && selectedItem + upDown < MENU_LENGTH) {
		selectedItem += upDown;
		refreshScreen();
	}
}

Screen *ConfigureScreen::onShortClick() {
	return menu[selectedItem].screen;
}

void SetDeviceIDScreen::onEnter() {
	refreshScreen();
}

Screen *SetDeviceIDScreen::onShortClick()
{
	return this;
}

Screen *SetDeviceIDScreen::onUpDown(int upDown)
{
	return this;
}

Screen *SetDeviceIDScreen::onLeftRight(int leftRight)
{
	return this;
}


void SetDeviceIDScreen::refreshScreen()
{
	display.fillScreen(BLACK);
	display.setTextSize(3);
	display.setCursor(20, 20);
}