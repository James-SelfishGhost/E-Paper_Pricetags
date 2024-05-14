#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>



#ifdef HAS_TFT

#include "ips_display.h"

#define YELLOW_SENSE 8  // sense AP hardware
#define TFT_BACKLIGHT 14

TFT_eSPI tft2 = TFT_eSPI();
uint8_t YellowSense = 0;
bool tftLogscreen = true;
bool tftOverride = false;

void TFTLog(String text) {
    if (tftLogscreen == false) {
        tft2.fillScreen(TFT_BLACK);
        tft2.setCursor(0, 0, (tft2.width() == 160 ? 1 : 2));
        tftLogscreen = true;
    } else {
        if (tft2.width() == 160) tft2.setCursor(0, tft2.getCursorY(), 1);
    }
    if (text.isEmpty()) return;
    tft2.setTextColor(TFT_SILVER);
    if (text.startsWith("!")) {
        tft2.setTextColor(TFT_RED);
        text = text.substring(1);
    } else if (text.indexOf("http") != -1) {
        int httpIndex = text.indexOf("http");
        tft2.print(text.substring(0, httpIndex));
        tft2.setTextColor(TFT_YELLOW);
        if (tft2.width() == 160) {
            tft2.setCursor(0, tft2.getCursorY() + 8, 2);
            text = text.substring(httpIndex + 7);
        } else {
            text = text.substring(httpIndex);
        }
    } else if (text.indexOf(":") != -1) {
        int colonIndex = text.indexOf(":");
        tft2.setTextColor(TFT_SILVER);
        tft2.print(text.substring(0, colonIndex + 1));
        tft2.setTextColor(TFT_WHITE);
        if (tft2.width() == 160) tft2.setCursor(0, tft2.getCursorY() + 8, 2);
        text = text.substring(colonIndex + 1);
    } else if (text.endsWith("!")) {
        tft2.setTextColor(TFT_GREEN);
    }
    tft2.println(text);
}



void yellow_ap_display_init(void) {
    pinMode(YELLOW_SENSE, INPUT_PULLDOWN);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (digitalRead(YELLOW_SENSE) == HIGH) YellowSense = 1;
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, LOW);

    tft2.init();
    tft2.setRotation(YellowSense == 1 ? 1 : 3);
    tft2.fillScreen(TFT_BLACK);
    tft2.setCursor(12, 0, (tft2.width() == 160 ? 1 : 2));
    tft2.setTextColor(TFT_WHITE);
    tftLogscreen = true;

    ledcSetup(6, 5000, 8);
    ledcAttachPin(TFT_BACKLIGHT, 6);
    if (tft2.width() == 160) {
        GPIO.func_out_sel_cfg[TFT_BACKLIGHT].inv_sel = 1;
    }
    ledcWrite(6, 255);
}

void yellow_ap_display_loop(void) {
    static bool first_run = 0;
    static time_t last_checkin = 0;
    static time_t last_update = 0;

    if (millis() - last_checkin >= 60000) {
        last_checkin = millis();
        tftLogscreen = false;
    }
    if (first_run == 0) {
        first_run = 1;
    }
    if (millis() - last_update >= 3000) {
        uint8_t wifimac[8];
        WiFi.macAddress(wifimac);
        memset(&wifimac[6], 0, 2);
        last_update = millis();
    }
}

#endif