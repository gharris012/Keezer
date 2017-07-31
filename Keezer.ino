#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "button.h"

#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define DBG_OUTPUT_PORT Serial

#define MODE_ON 1
#define MODE_OFF 2
#define MODE_AUTO 4

/* pinout
 *
 *  4, 5 - i2c
 *  0, 16, 2 - oled buttons
 *  12 - ds temp
 *  15 - powerswitch tail (built-in pulldown)
 *
 */

#ifdef WIFI_SSID
String wifi_ssid = WIFI_SSID;
#else
String wifi_ssid = "unk";
#endif

#ifdef WIFI_PASSWORD
String wifi_password = WIFI_PASSWORD;
#else
String wifi_password = "unk";
#endif

#ifdef WIFI_HOSTNAME
String wifi_host = WIFI_HOSTNAME;
#else
String wifi_host = "esp";
#endif

const byte ownPin = 12;
OneWire own(ownPin);

const uint8_t ON = 1;
const uint8_t OFF = 0;

uint8_t MAX_TEMPERATURE = 80;
uint8_t MIN_TEMPERATURE = 32;
const uint8_t DEFAULT_TEMPERATURE = 38;
int keezer_min_on_time = 5*60000;
int keezer_min_off_time = 5*60000;
uint8_t keezer_temperature_threshold = 2; // +- 2 degrees

uint8_t keezer_target_temperature = 70;
uint8_t keezer_state = OFF;
uint8_t keezer_pin = 15;
uint8_t keezer_mode = MODE_AUTO;
unsigned long int keezer_timer_last = 0;

const float INVALID_TEMPERATURE = -123;
const byte DS_TEMP_SENSOR_CONVERT_DURATION = 250;
const unsigned long int DS_TEMP_GRACE_PERIOD = 60000; // if we haven't gotten a valid temp in this amount of time, mark it as disconnected
const byte DS_SENSOR_COUNT = 1;
const byte DS_KEEZER = 0;

typedef struct DSTempSensor
{
    char name[10];
    uint8_t addr[8];

    float tempF;

    float last_tempF;
    int last_valid_read;
    bool present;
} DSTempSensor;

DSTempSensor ds_temp_sensor[DS_SENSOR_COUNT] = {
    {
        "Keezer",
        {0x28, 0xFF, 0x63, 0x70, 0xA3, 0x16, 0x5, 0xAF},
        1, NULL,
        INVALID_TEMPERATURE, FALSE
    }
};

bool rescanOWN = FALSE;

bool ds_temp_sensor_is_converting = FALSE;
unsigned long int ds_temp_sensor_convert_complete_time = 0;

// read temperatures every second
unsigned long int read_temperatures_next_time = 0;
const int read_temperatures_delay = 1000;

// update the display every 0.5 seconds
const int update_display_delay = 500;
unsigned long int update_display_next_time = update_display_delay;

// update the keezer control five seconds
const int keezer_check_delay = 5000;
unsigned long int keezer_check_next_time = keezer_check_delay;

Adafruit_SSD1306 display = Adafruit_SSD1306();

#define BUTTON_COUNT 3
Button buttons[BUTTON_COUNT] = {
    {"A", 0 },
    {"B", 16 },
    {"C", 2 },
};

ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

#define LED      0

#if (SSD1306_LCDHEIGHT != 32)
    #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()
{
    Serial.begin(115200);

    pinMode(keezer_pin, OUTPUT);
    digitalWrite(keezer_pin, LOW);

    delay(1000);

    Serial.println("Keezer Controller v0.2");
    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
    display.setTextWrap(FALSE);
    // init done

    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    delay(1000);

    scanOWN();
    // set resolution on all DS temp sensors
    own.reset();
    own.skip();
    own.write(0x4E);         // Write scratchpad
    own.write(0);            // TL
    own.write(0);            // TH
    own.write(0x3F);         // 10-bit resolution
    own.write(0x48);         // Copy Scratchpad
    own.reset();

    // Clear the buffer.
    display.clearDisplay();
    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(WHITE);
    display.display();

    Serial.println("Setting up buttons");
    setup_buttons(buttons, BUTTON_COUNT);

    Serial.println("Ready");
}

void loop()
{
    check_buttons(buttons, BUTTON_COUNT);
    if ( millis() > read_temperatures_next_time )
    {
        read_temperatures();
        read_temperatures_next_time += read_temperatures_delay;
    }
    if ( ds_temp_sensor_is_converting && millis() > ds_temp_sensor_convert_complete_time )
    {
        read_ds_temperatures();
    }
    if ( millis() > update_display_next_time )
    {
        update_display();
        update_display_next_time += update_display_delay;
    }
    if ( millis() > keezer_check_next_time )
    {
        check_keezer();
        keezer_check_next_time += keezer_check_delay;
    }
    yield();
}

void update_display()
{
    float tempF = ds_temp_sensor[DS_KEEZER].tempF;
    int tempFi = (int) tempF;
    int tempFm;

    display.clearDisplay();

    // display current temperature
    display.setFont(&FreeSansBold24pt7b);
    display.setCursor(0,32);
    if ( tempF == INVALID_TEMPERATURE )
    {
        Serial.print(" invalid temperature");
        display.print("nc");
    }
    else
    {
        tempFm = ( tempF * 10 );
        tempFm = tempFm % 10;
        display.print(tempFi);
        display.print(".");
        display.print(tempFm);
    }

    int16_t  x1, y1;
    uint16_t w, h;
    char buf[4] = "";

    // display keezer target temperature in lower right
    snprintf(buf, sizeof(buf), "%d", keezer_target_temperature);
    display.setFont(&FreeSans12pt7b);
    display.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
    display.setCursor(128 - 1 - w, 32);
    display.print(buf);

    // display keezer state in upper right
    if ( keezer_state )
    {
        snprintf(buf, sizeof(buf), "ON");
    }
    else
    {
        snprintf(buf, sizeof(buf), "OFF");
    }
    display.setFont(&FreeSans9pt7b);
    display.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
    display.setCursor(128 - 1 - w, h);
    display.print(buf);

    display.display();
}

void check_keezer()
{
    Serial.println("checking Keezer");
    uint8_t state = OFF;
    uint8_t tempF = ds_temp_sensor[DS_KEEZER].tempF;

    if ( ds_temp_sensor[DS_KEEZER].present )
    {
        if ( keezer_mode == MODE_AUTO )
        {
            if ( ( keezer_state == OFF && tempF > ( keezer_target_temperature + keezer_temperature_threshold ) )
                || ( keezer_state == ON && tempF > ( keezer_target_temperature - keezer_temperature_threshold ) ) )
            {
                state = ON;
            }
            else
            {
                state = OFF;
            }
        }
        else if ( keezer_mode == MODE_OFF )
        {
            state = OFF;
        }
        else
        {
            Serial.print("Unknown mode: ");
            Serial.println(keezer_mode);
            state = OFF;
        }
    }
    else
    {
        state = OFF;
    }
    Serial.print("Prefilter state: ");
    Serial.println(state);

    // Filter state to ensure on/off min times
    //  allow some cushion for initial startup
    if ( keezer_state != state && keezer_timer_last > 5000 )
    {
        unsigned long int next_available_time = 0;
        if ( keezer_state == ON
                && keezer_timer_last + keezer_min_on_time > millis() )
        {
            next_available_time = keezer_timer_last + keezer_min_on_time;
            state = TRUE;
        }

        if ( keezer_state == OFF
                && keezer_timer_last + keezer_min_off_time > millis() )
        {
            next_available_time = keezer_timer_last + keezer_min_off_time;
            state = OFF;
        }

        if ( keezer_state == state )
        {
            Serial.print(" Overriding due to min on/off time: ");
            Serial.print(millis());
            Serial.print(" ; next time: ");
            Serial.println(next_available_time);
        }
    }

    if ( keezer_state == ON && state == ON && tempF <= MIN_TEMPERATURE )
    {
        Serial.println(" temperature too low, turning off");
        state = OFF;
    }

    Serial.print("final state decision tree: ");
    Serial.print(state);
    Serial.print(" ; timer_last: ");
    Serial.println(keezer_timer_last);
    if ( keezer_state != state || keezer_timer_last == 0 )
    {
        Serial.print("Updating keezer state: ");
        Serial.println(state);
        keezer_state = state;
        keezer_timer_last = millis();
    }
}
