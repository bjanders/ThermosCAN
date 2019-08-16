#pragma once

#include <OneWireExt.h>

const int MAX_OW_DEVICES = 5;

struct Config {
	unsigned short version;
	byte mainOwAddress[OW_ADDRESS_LEN]; // Main 1-wire device address, shown on display
	short setTemp; // Target temperature
	unsigned short setTempCANId; // CAN ID for target temperature
	unsigned short owCANId[MAX_OW_DEVICES]; // CAN ID for 1-wire devices
};

void setDisplayContrast(byte contrast);