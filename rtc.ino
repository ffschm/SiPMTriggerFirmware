#include <Wire.h>

void setup_rtc() {
    Wire.begin();
    rtc.begin();

    if (! rtc.isrunning()) {
        Serial.println("# RTC is NOT running!");
    }

    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
