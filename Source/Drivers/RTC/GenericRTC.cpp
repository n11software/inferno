//========= Copyright N11 Software, All rights reserved. ============//
//
// File: genericrtc.cpp
// Purpose: Generic RTC driver
// Maintainer: aristonl
//
//===================================================================//

#include <Inferno/IO.h>
#include <Drivers/CMOS/CMOS.h>
#include <Drivers/RTC/RTC.h>

#include <Drivers/TTY/COM.h>

#include <Inferno/Log.h>

unsigned char second; // 0x00
unsigned char minute; // 0x02
unsigned char hour;  // 0x04

unsigned char day; // 0x07
unsigned char month; // 0x08
unsigned int year; // 0x09

int century_register = 0x00;

unsigned char registerB; // 0x0B

namespace RTC {

void readRTC() {
	second = getRegister(0x00);
	minute = getRegister(0x02);
	hour = getRegister(0x04);

	day = getRegister(0x07);
	month = getRegister(0x08);
	year = getRegister(0x09);

	registerB = getRegister(0x0B);

	// Convert BCD to binary values if necessary

	if (!(registerB & 0x04)) {
		second = (second & 0x0F) + ((second / 16) * 10);
		minute = (minute & 0x0F) + ((minute / 16) * 10);
		hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
		day = (day & 0x0F) + ((day / 16) * 10);
		month = (month & 0x0F) + ((month / 16) * 10);
		year = (year & 0x0F) + ((year / 16) * 10);
	}

	// Convert 12 hour clock to 24 hour clock if necessary

	if (!(registerB & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}

	unsigned int century = 20; // Hardcoded since Hydra probwon't be around in 2100.
	unsigned int full_year = (century * 100) + year;

	// FIXME: Make the hour, min, sec show as two digits when it is < 10. (ex: 07 instead of 7)
	prInfo("rtc", "%d/%d/%dT%d:%d:%d", day, month, full_year, hour, minute, second);
}

int init() {
	RTC::readRTC();
	prInfo("rtc", "rtc initialized");
	return 0;
}

}
