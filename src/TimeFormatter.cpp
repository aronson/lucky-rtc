#include "TimeFormatter.h"

bn::string<33> TimeFormatter::renderLine() {
    readLine.clear();

    addComponent(formatNumber(year), Year);
    readLine += "/";
    addComponent(formatNumber(month), Month);
    readLine += "/";
    addComponent(formatNumber(day), Day);
    readLine += " ";

    if (!(rtcStatus & 0x40)) {
        int displayHour = hour % 12;
        if (displayHour == 0) displayHour = 12;
        addComponent(formatNumber(displayHour), Hour);
    } else {
        addComponent(formatNumber(hour), Hour);
    }

    readLine += ":";
    addComponent(formatNumber(minute), Minute);
    readLine += ":";
    addComponent(formatNumber(second), Second);

    if (!(rtcStatus & 0x40)) {
        readLine += " ";
        addComponent(afternoon ? "PM" : "AM", Afternoon);
    }

    return readLine;
}
