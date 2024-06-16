#include <Drivers/CMOS/CMOS.h>
#include <Drivers/RTC/RTC.h>

unsigned long long RTC::getEpochTime() {
    unsigned int second = getRegister(0x00);
    unsigned int minute = getRegister(0x02);
    unsigned int hour = getRegister(0x04);
    unsigned int day = getRegister(0x07);
    unsigned int month = getRegister(0x08);
    unsigned int year = getRegister(0x09);

    if (month < 3) {
        month += 12;
        year -= 1;
    }

    unsigned long long epochTime = (second + minute * 60 + hour*3600 + day*86400 +
        (month - 2)*2630016/12 + year*31556916);

    return epochTime;
}