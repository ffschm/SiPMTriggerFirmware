#include <LiquidCrystal.h>
#include "RTClib.h"

void setup_lcd() {
    lcd.begin(16, 4);
    lcd1.begin(16, 2);

    lcd1.setCursor(0, 0);
    lcd1.print("SiPMTrigger Control");
    lcd1.setCursor(0, 1);
    lcd1.print("HW v4    SW v");
    lcd1.print(SW_VERSION);
}


void update_lcd() {
    lcd.clear();
    show_time();
    lcd_print_interrupts();
}

void show_time() {
    if (! rtc.isrunning()) {
      lcd.setCursor(0,0); // first line
      lcd.print("RTC not running");
      delay(1000);
      return;
    }
    DateTime now = rtc.now();

    lcd.setCursor(0,0); // first line
    lcd.print(now.day(), DEC);
    lcd.print('.');
    lcd.print(now.month(), DEC);
    lcd.print('.');
    lcd.print("  ");

    if (now.day() < 10) {
        lcd.print(' ');
    }
    if (now.month() < 10) {
        lcd.print(' ');
    }
    if (now.hour() < 10) {
        lcd.print(' ');
    }
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute() < 10) {
        lcd.print('0');
    }
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    if (now.second() < 10) {
        lcd.print('0');
    }
    lcd.print(now.second(), DEC);
}
