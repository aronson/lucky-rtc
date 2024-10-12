#pragma once

#include "bn_string.h"

class TimeFormatter {
private:
    bn::string<32> readLine;
    int year, month, day, hour, minute, second;
    bool afternoon;
    int selectedComponent;
    int rtcStatus;

    static bn::string<2> formatNumber(int num) {
        bn::string<2> result = bn::to_string<2>(num);
        if (result.length() < 2) {
            result = bn::string<2>(2 - result.length(), '0') + result;
        }
        return result;
    }

    void addComponent(const bn::string<33> &value, int component) {
        if (selectedComponent == component) readLine += "<";
        readLine += value;
        if (selectedComponent == component) readLine += ">";
    }

public:
    enum Component {
        Year, Month, Day, Hour, Minute, Second, Afternoon
    };

    TimeFormatter(int y, int mo, int d, int h, int mi, int s, bool pm, int selected, int status)
            : year(y), month(mo), day(d), hour(h), minute(mi), second(s),
              afternoon(pm), selectedComponent(selected), rtcStatus(status) {}

    bn::string<33> renderLine();
};

