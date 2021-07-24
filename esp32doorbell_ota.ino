////////////LIBRARIES////////////
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#if defined(OTAENABLE)
#include <ArduinoOTA.h>
#endif
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>

////////////DEFINITIONS////////////
const char* ssid = "YOUR SSID";
const char* password = "YOUR PSWD";
String BOTtoken = "YOUR BOT TOKEN";  // your Bot Token (Get from Botfather)
String CHAT_ID = "YOUR ID";
String CHAT_ID1 = "YOUR ID";
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
unsigned long bot_lasttime = 0 ;
const unsigned long BOT_MTBS = 4000; //for how much time should the msgs be checked for
bool sendPhoto = false;
bool flashState = LOW;
#define bellpin 12

WiFiClientSecure clientTCP;
WiFiClientSecure BOTtcp;
UniversalTelegramBot bot(BOTtoken, BOTtcp);

//#define OTAENABLE  // un-comment this line for enabling ota updates
//CAMERA_MODEL_AI_THINKER
#define FLASH_LED_PIN 4
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


////////////FUNCTION DECLARATION////////////
void configInitCamera();
String sendPhotoTelegram() ;
void handleNewMessages(int);
void blinkLED(int);
void configmode();



////////////FUNCTIONS////////////


void blinkLED(int n)
{
  if (n == 0)
  {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(50);
    digitalWrite(FLASH_LED_PIN, LOW);
  } else if (n == 1)
  {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(100);
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_LED_PIN, LOW);
  }
}


void setup() {

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200); // serial monitor initiallisation
  pinMode(bellpin, INPUT_PULLUP); // defining bellpin as input with pullup
  pinMode(FLASH_LED_PIN, OUTPUT); // Set LED Flash as output
  digitalWrite(FLASH_LED_PIN, flashState);

  WiFi.mode(WIFI_STA); // set mode as station
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");    // check if connected or not
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); // print IP
  configInitCamera(); //initialize camera


  // Config OTA
#if defined(OTAENABLE)
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));

  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname("ESP32 doorbell");
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.begin();
#endif
}

void loop() {
  if (sendPhoto)
  {
    Serial.println("Preparing photo");
    //sendPhotoTelegram(CHAT_ID);
    if (CHAT_ID1 != "")
    {
      sendPhotoTelegram(CHAT_ID1);
    }
    sendPhoto = false;
  } else if (digitalRead(12) == LOW )
  {
    Serial.println("bell Pressed");
    //blinkLED(0);
    //sendPhotoTelegram(CHAT_ID);
    if (CHAT_ID1 != "")
    {
      sendPhotoTelegram(CHAT_ID1);
    }
    sendPhoto = false;
    bot_lasttime = millis();
  }

#if defined(OTAENABLE)
  else
  {
    ArduinoOTA.handle();
  }
#endif

  if (millis() - bot_lasttime < BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}
