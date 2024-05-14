#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>
#include "main_variables.h"
#include "RFV3.h"
#include "trans_assist.h"

#ifdef HAS_TFT

#include "ips_display.h"

#define YELLOW_SENSE 8  // sense AP hardware
#define TFT_BACKLIGHT 14

TFT_eSPI tft2 = TFT_eSPI();
uint8_t YellowSense = 0;
bool tftLogscreen = true;
bool tftOverride = false;
const int MAX_LINES = 4; // Maximum number of lines to display on the left side
String lines[MAX_LINES]; // Array to store the lines
int lineCount = 0; // Current number of lines
int lastSentId = 0; // Variable to store the last sent ID

// Variables to store the previous values
String prevSendStatus = "";
String prevWaiting = "";
String prevActiStatus = "";
String prevNetID = "";
String prevFreq = "";
String prevSlot = "";
String prevBytesLeft = "";
String prevOpen = "";
String prevLastAnswer = "";
String prevMode = "";

void TFTLog(String text) {
    // Update the lines array
    if (lineCount < MAX_LINES) {
        lines[lineCount++] = text;
    } else {
        // Shift the lines up by removing the oldest entry
        for (int i = 0; i < MAX_LINES - 1; i++) {
            lines[i] = lines[i + 1];
        }
        lines[MAX_LINES - 1] = text;
    }

    // Clear the left side of the screen
    tft2.fillRect(0, 0, tft2.width() / 2, tft2.height(), TFT_BLACK);

    // Display the lines on the left side
    tft2.setCursor(0, 0);
    for (int i = 0; i < lineCount; i++) {
        tft2.println(lines[i]);
    }

    // Display the "Last Sent ID" on the right side
    // tft2.fillRect(tft2.width() / 2, 0, tft2.width() / 2, tft2.height(), TFT_BLACK);
    // tft2.setCursor(tft2.width() / 2, 0);
    // tft2.print("Last Sent ID: ");
    // tft2.println(lastSentId);
}

void setLastSentId(int id) {
    lastSentId = id;
}



void publishModeTFT() {
    String actiStatus = "";
    String sendStatus = "";

    switch (get_last_activation_status()) {
        case 0:
            actiStatus = "not started";
            break;
        case 1:
            actiStatus = "started";
            break;
        case 2:
            actiStatus = "timeout";
            break;
        case 3:
            actiStatus = "successful";
            break;
        default:
            actiStatus = "Error";
            break;
    }

    switch (get_last_send_status()) {
        case 0:
            sendStatus = "nothing send";
            break;
        case 1:
            sendStatus = "in sending";
            break;
        case 2:
            sendStatus = "timeout";
            break;
        case 3:
            sendStatus = "successful";
            break;
        default:
            sendStatus = "Error";
            break;
    }

    String waiting = String(get_is_data_waiting_raw());
    String netID = String(get_network_id());
    String freq = String(get_freq());
    String slot = String(get_slot_address());
    String bytesLeft = String(get_still_to_send());
    String open = String(get_trans_file_open());
    String lastAnswer = get_last_receive_string();
    String mode = get_mode_string();

    // Update the screen only if a value has changed
    if (sendStatus != prevSendStatus) {
        tft2.fillRect(tft2.width() / 2, 0, tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 0);
        tft2.print("Send: ");
        tft2.println(sendStatus);
        prevSendStatus = sendStatus;
    }

    if (waiting != prevWaiting) {
        tft2.fillRect(tft2.width() / 2, tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, tft2.fontHeight());
        tft2.print("waiting: ");
        tft2.println(waiting);
        prevWaiting = waiting;
    }

    if (actiStatus != prevActiStatus) {
        tft2.fillRect(tft2.width() / 2, 2 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 2 * tft2.fontHeight());
        tft2.print("Activation: ");
        tft2.println(actiStatus);
        prevActiStatus = actiStatus;
    }

    if (netID != prevNetID) {
        tft2.fillRect(tft2.width() / 2, 3 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 3 * tft2.fontHeight());
        tft2.print("NetID ");
        tft2.println(netID);
        prevNetID = netID;
    }

    if (freq != prevFreq) {
        tft2.fillRect(tft2.width() / 2, 4 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 4 * tft2.fontHeight());
        tft2.print("freq ");
        tft2.println(freq);
        prevFreq = freq;
    }

    if (slot != prevSlot) {
        tft2.fillRect(tft2.width() / 2, 5 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 5 * tft2.fontHeight());
        tft2.print("slot ");
        tft2.println(slot);
        prevSlot = slot;
    }

    if (bytesLeft != prevBytesLeft) {
        tft2.fillRect(tft2.width() / 2, 6 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 6 * tft2.fontHeight());
        tft2.print("bytes left: ");
        tft2.println(bytesLeft);
        prevBytesLeft = bytesLeft;
    }

    if (open != prevOpen) {
        tft2.fillRect(tft2.width() / 2, 7 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 7 * tft2.fontHeight());
        tft2.print("Open: ");
        tft2.println(open);
        prevOpen = open;
    }

    if (lastAnswer != prevLastAnswer) {
        tft2.fillRect(tft2.width() / 2, 8 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 8 * tft2.fontHeight());
        tft2.print("last answer: ");
        tft2.println(lastAnswer);
        prevLastAnswer = lastAnswer;
    }

    if (mode != prevMode) {
        tft2.fillRect(tft2.width() / 2, 9 * tft2.fontHeight(), tft2.width() / 2, tft2.fontHeight(), TFT_BLACK);
        tft2.setCursor(tft2.width() / 2, 9 * tft2.fontHeight());
        tft2.print("mode ");
        tft2.println(mode);
        prevMode = mode;
    }
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