/*
    An updated clock combining an analogue and digital display using a TFT LCD screen.
    Features a black starry sky background covering the entire screen, NTP synchronization, WiFi status messages,
    and a button to change time zones.
    Uses TFT_eSPI library for ST7735 driver chip.

    Analogue clock, digital clock, and WiFi status text have configurable integer center points.
    All elements move as a group when center points are changed.
    All parameters are integers to avoid floating-point errors.
    Removed "Colour" text and hex code display.

    Original analogue clock by Gilchrist 6/2/2014 1.0, updated by Bodmer.
    Digital clock based on Bodmer's example.
    Modified for NTP sync, starry background, dual clocks, center points, full star coverage, and time zone button on 2025-04-16.
*/

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <WiFi.h>     // WiFi library for ESP32/ESP8266
#include <NTPClient.h> // NTP client library
#include <WiFiUdp.h>   // UDP for NTP

// WiFi credentials (replace with your own)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC offset 0 initially, update interval 60s

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// Center points (integer)
int16_t analogCenterX = 123;  // Analogue clock center X
int16_t analogCenterY = 64;   // Analogue clock center Y
int16_t digitalCenterX = 73;  // Digital clock center X
int16_t digitalCenterY = 190; // Digital clock center Y
int16_t textCenterX = 80;     // WiFi status text center X
int16_t textCenterY = 280;    // WiFi status text center Y

// Button setup
const int buttonPin = 4; // GPIO pin for time zone button
uint32_t lastButtonPress = 0; // For debouncing
const uint32_t debounceDelay = 200; // Debounce interval (ms)

// Time zones
struct TimeZone {
    int32_t offset; // UTC offset in seconds
    const char* label; // Display label
};
const TimeZone timeZones[] = {
    {-18000, "UTC-5"}, // UTC-5 (e.g., Eastern Standard Time)
    {0, "UTC"},        // UTC
    {28800, "UTC+8"}   // UTC+8 (e.g., China Standard Time)
};
const int numTimeZones = 3;
int tzIndex = 1; // Start with UTC

// Analogue clock variables
int16_t osx = 123, osy = 64, omx = 123, omy = 64, ohx = 123, ohy = 64; // Saved H, M, S coords
int16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
boolean initial = 1;

// Digital clock variables
byte omm = 99;
int16_t xcolon = 0;

// Shared variables
uint32_t targetTime = 0;                    // for next 1 second timeout
uint32_t lastNtpSync = 0;                   // for NTP sync every 60 seconds
bool wifiConnected = false;                  // WiFi status
uint8_t hh = 0, mm = 0, ss = 0;             // Shared H, M, S
uint32_t tzDisplayUntil = 0;                // When to revert time zone display to WiFi status

// Integer sine/cosine lookup table (degrees 0-360, scaled by 1000)
const int16_t sinTable[61] = {
    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 994, 1000, 994, 978, 951, 914,
    866, 809, 743, 669, 588, 500, 407, 309, 208, 105, 0, -105, -208, -309, -407, -500, -588, -669, -743,
    -809, -866, -914, -951, -978, -994, -1000, -994, -978, -951, -914, -866, -809, -743, -669, -588, -500,
    -407, -309, -208, -105, 0
}; // sin(0) to sin(360), step 6 degrees

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9') {
        v = *p - '0';
    }
    return 10 * v + *++p - '0';
}

void setup(void) {
    // Initialize time from compile time
    hh = conv2d(__TIME__);
    mm = conv2d(__TIME__ + 3);
    ss = conv2d(__TIME__ + 6);

    // Initialize button
    pinMode(buttonPin, INPUT_PULLUP);

    // Initialize TFT
    tft.init();
    tft.setRotation(1); // Landscape orientation (160x128)
    tft.fillScreen(TFT_BLACK); // Black background for starry sky
    tft.setTextColor(TFT_GREEN, TFT_BLACK); // Green text on black background

    // Draw starry sky (random white pixels across entire screen)
    for (int i = 0; i < 80; i++) {
        int16_t x = random(0, 160);
        int16_t y = random(0, 128);
        tft.drawPixel(x, y, TFT_WHITE);
    }

    // Display "Searching WiFi"
    tft.drawCentreString("Searching WiFi", textCenterX, textCenterY, 4);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    tft.fillRect(textCenterX - 80, textCenterY - 5, 160, 20, TFT_BLACK); // Clear text area
    tft.drawCentreString("Connecting WiFi", textCenterX, textCenterY, 4);
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
        delay(500);
        wifiAttempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        timeClient.begin();
        // Initial NTP sync
        if (timeClient.update()) {
            setTimeFromNTP();
        } else {
            displaySyncStatus(false);
        }
    } else {
        wifiConnected = false;
        displaySyncStatus(false);
    }

    // Draw analogue clock face
    tft.fillCircle(analogCenterX, analogCenterY, 61, TFT_BLUE);
    tft.fillCircle(analogCenterX, analogCenterY, 57, TFT_BLACK);

    // Draw 12 lines
    for (int16_t i = 0; i < 360; i += 30) {
        int16_t angleIdx = (i / 6) % 60;
        int16_t sx = sinTable[angleIdx];
        int16_t sy = sinTable[(angleIdx + 15) % 60]; // sin = cos(90-angle)
        x0 = (sx * 57) / 1000 + analogCenterX;
        yy0 = (sy * 57) / 1000 + analogCenterY;
        x1 = (sx * 50) / 1000 + analogCenterX;
        yy1 = (sy * 50) / 1000 + analogCenterY;
        tft.drawLine(x0, yy0, x1, yy1, TFT_BLUE);
    }

    // Draw 60 dots
    for (int16_t i = 0; i < 360; i += 6) {
        int16_t angleIdx = (i / 6) % 60;
        int16_t sx = sinTable[angleIdx];
        int16_t sy = sinTable[(angleIdx + 15) % 60];
        x0 = (sx * 53) / 1000 + analogCenterX;
        yy0 = (sy * 53) / 1000 + analogCenterY;
        tft.drawPixel(x0, yy0, TFT_BLUE);
        if (i == 0 || i == 180) {
            tft.fillCircle(x0, yy0, 1, TFT_CYAN);
            tft.fillCircle(x0 + 1, yy0, 1, TFT_CYAN);
        }
        if (i == 90 || i == 270) {
            tft.fillCircle(x0, yy0, 1, TFT_CYAN);
            tft.fillCircle(x0 + 1, yy0, 1, TFT_CYAN);
        }
    }

    tft.fillCircle(analogCenterX, analogCenterY, 3, TFT_RED);

    // Draw digital clock initial elements
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(digitalCenterX - 32, digitalCenterY + 20);
    tft.print(__DATE__); // Standard small font

    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.drawCentreString("It is windy", digitalCenterX + 40, digitalCenterY + 16, 2); // Font 2

    targetTime = millis() + 1000;
}

void loop() {
    // Check button for time zone change
    if (digitalRead(buttonPin) == LOW && millis() - lastButtonPress > debounceDelay) {
        lastButtonPress = millis();
        tzIndex = (tzIndex + 1) % numTimeZones; // Cycle to next time zone
        timeClient.setTimeOffset(timeZones[tzIndex].offset);
        if (wifiConnected && timeClient.update()) {
            setTimeFromNTP();
        }
        // Display time zone briefly
        tft.fillRect(textCenterX - 80, textCenterY - 5, 160, 20, TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString(timeZones[tzIndex].label, textCenterX, textCenterY, 4);
        tzDisplayUntil = millis() + 2000; // Show for 2 seconds
    }

    // Revert to WiFi status after time zone display
    if (tzDisplayUntil != 0 && millis() > tzDisplayUntil) {
        displaySyncStatus(wifiConnected);
        tzDisplayUntil = 0;
    }

    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiConnected) {
            wifiConnected = false;
            displaySyncStatus(false);
        }
    } else if (!wifiConnected) {
        wifiConnected = true;
        timeClient.begin();
        displaySyncStatus(true);
    }

    // Update time every second
    if (targetTime < millis()) {
        targetTime = millis() + 1000;
        ss++;              // Advance second
        if (ss == 60) {
            ss = 0;
            mm++;            // Advance minute
            if (mm > 59) {
                mm = 0;
                hh++;          // Advance hour
                if (hh > 23) {
                    hh = 0;
                }
            }
        }

        // Attempt NTP sync every 60 seconds
        if (wifiConnected && (millis() - lastNtpSync >= 60000)) {
            if (timeClient.update()) {
                setTimeFromNTP();
                displaySyncStatus(true);
            } else {
                displaySyncStatus(false);
            }
            lastNtpSync = millis();
        }

        // Update analogue clock
        int16_t sdeg = ss * 6; // 0-59 -> 0-354
        int16_t mdeg = mm * 6 + (sdeg * 167) / 10000; // 0-59 -> 0-360, includes seconds
        int16_t hdeg = hh * 30 + (mdeg * 833) / 10000; // 0-11 -> 0-360, includes minutes
        int16_t angleIdx;

        // Hour hand
        angleIdx = (hdeg / 6) % 60;
        int16_t hx = sinTable[angleIdx];
        int16_t hy = sinTable[(angleIdx + 15) % 60];
        if (ss == 0 || initial) {
            tft.drawLine(ohx, ohy, analogCenterX, analogCenterY, TFT_BLACK);
        }
        ohx = (hx * 33) / 1000 + analogCenterX;
        ohy = (hy * 33) / 1000 + analogCenterY;

        // Minute hand
        angleIdx = (mdeg / 6) % 60;
        int16_t mx = sinTable[angleIdx];
        int16_t my = sinTable[(angleIdx + 15) % 60];
        if (ss == 0 || initial) {
            tft.drawLine(omx, omy, analogCenterX, analogCenterY, TFT_BLACK);
        }
        omx = (mx * 44) / 1000 + analogCenterX;
        omy = (my * 44) / 1000 + analogCenterY;

        // Second hand
        angleIdx = (sdeg / 6) % 60;
        int16_t sx = sinTable[angleIdx];
        int16_t sy = sinTable[(angleIdx + 15) % 60];
        tft.drawLine(osx, osy, analogCenterX, analogCenterY, TFT_BLACK);
        osx = (sx * 47) / 1000 + analogCenterX;
        osy = (sy * 47) / 1000 + analogCenterY;

        if (ss == 0 || initial) {
            initial = 0;
        }

        // Redraw hands
        tft.drawLine(ohx, ohy, analogCenterX, analogCenterY, TFT_WHITE);
        tft.drawLine(omx, omy, analogCenterX, analogCenterY, TFT_WHITE);
        tft.drawLine(osx, osy, analogCenterX, analogCenterY, TFT_RED);

        tft.fillCircle(analogCenterX, analogCenterY, 3, TFT_RED);

        // Update digital clock
        int16_t xpos = digitalCenterX - 34;
        int16_t ypos = digitalCenterY - 32;
        if (omm != mm) { // Redraw every minute to minimise flicker
            tft.setTextColor(0x39C4, TFT_BLACK); // Ghost image
            tft.drawString("88:88", xpos, ypos, 7); // Overwrite to clear
            tft.setTextColor(0xFBE0, TFT_BLACK); // Orange
            omm = mm;

            if (hh < 10) {
                xpos += tft.drawChar('0', xpos, ypos, 7);
            }
            xpos += tft.drawNumber(hh, xpos, ypos, 7);
            xcolon = xpos;
            xpos += tft.drawChar(':', xpos, ypos, 7);
            if (mm < 10) {
                xpos += tft.drawChar('0', xpos, ypos, 7);
            }
            tft.drawNumber(mm, xpos, ypos, 7);
        }

        if (ss % 2) { // Flash the colon
            tft.setTextColor(0x39C4, TFT_BLACK);
            tft.drawChar(':', xcolon, ypos, 7);
            tft.setTextColor(0xFBE0, TFT_BLACK);
        } else {
            tft.drawChar(':', xcolon, ypos, 7);
        }
    }
}

// Function to set time from NTP
void setTimeFromNTP() {
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime(&epochTime);
    hh = ptm->tm_hour;
    mm = ptm->tm_min;
    ss = ptm->tm_sec;
    initial = 1; // Force redraw of analogue hands
    omm = 99;    // Force redraw of digital time
}

// Function to display sync and WiFi status
void displaySyncStatus(bool syncSuccess) {
    // Skip if showing time zone
    if (tzDisplayUntil != 0 && millis() < tzDisplayUntil) {
        return;
    }
    // Clear previous text by drawing a rectangle
    tft.fillRect(textCenterX - 80, textCenterY - 5, 160, 20, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    if (!wifiConnected) {
        tft.drawCentreString("WiFi Disconnected", textCenterX, textCenterY, 4);
    } else if (!syncSuccess) {
        tft.drawCentreString("Sync Failed", textCenterX, textCenterY, 4);
    } else {
        tft.drawCentreString("WiFi Connected", textCenterX, textCenterY, 4);
    }
}