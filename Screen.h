// Screen.h

#ifndef _SCREEN_h
#define _SCREEN_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


// Screen dimensions
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 128;
const int ROW_HEIGHT = 8;
const int MENU_TEXT_SIZE = 1;
const int CHARS_PER_LINE = 21;

const byte DEG = 248; // Degrees symbol
// Color definitions
const int BLACK = 0x0000;
const int BLUE = 0x001F;
const int RED = 0xF800;
const int GREEN = 0x07E0;
const int CYAN = 0x07FF;
const int MAGENTA = 0xF81F;
const int YELLOW = 0xFFE0;
const int WHITE = 0xFFFF;


void saveConfig();

class Screen
{
public:
	virtual void onEnter();
	virtual void onExit();
	virtual Screen *onLongClick();
	virtual Screen *onShortClick();
	virtual Screen *onLeftRight(int leftRight);
	virtual Screen *onUpDown(int upDown);
	virtual Screen *onLoop(unsigned long millis);
};


class DefaultScreen : public Screen
{
public:
	void onEnter() override;
	Screen *onLoop(unsigned long millis);
	Screen *onShortClick();
	Screen *onLongClick();
protected:
	char tempDisplayedStr[10];
	short tempDisplayed;
	void refreshTemp();
	//void readTemp(unsigned long millis);
};

extern DefaultScreen defaultScreen;

class SetTempScreen : public Screen
{
public:
	void onEnter();
	Screen *onShortClick();
	Screen *onUpDown(int upDown);
protected:
	void refreshScreen();
};

extern SetTempScreen setTempScreen;

struct MenuItem {
	char text[21];
	Screen *screen;
};

class SetDeviceIDScreen : public Screen
{
public:
	void onEnter();
	Screen *onShortClick();
	Screen *onLongClick();
	Screen *onUpDown(int upDown);
	Screen *onLeftRight(int leftRight);
	void setInfo(char *text, unsigned short *id);
protected:
	static const int TEXT_LEN = 20;
	char text[TEXT_LEN];
	int leftRight;
	byte nibble;
	unsigned short *configId;
	unsigned short workId;
	void refreshScreen();
};

extern SetDeviceIDScreen setDeviceIDScreen;

class ConfigureScreen : public Screen
{
public:
	void onEnter();
	Screen *onShortClick();
	Screen *onLongClick();
	Screen *onUpDown(int upDown);
protected:
	int selectedItem;
	void refreshScreen();
};

extern ConfigureScreen configureScreen;

#endif

