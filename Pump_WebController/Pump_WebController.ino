//MODIFIED BY ALEPIRO85 - 2023-02-04
//BASED ON ORIGINAL CODE BY WILLIAMG42
//ON THE FORK OF THE MASTER OF EVERYTHING R0BB10

#define VERSION "0.1B"

//NOTES: IF AN EXTERNAL MEMORY IS USED, AS THE ORIGINAL PROJECT WANTS, YOU NEED TO DEFINE EXT_MEMORY (ACTUALLY NOT WORKING, HAVE TO BE DEBUGGED, IF YOU WANT TO DO IT UNCOMMENT NEXT CODE LINE)
//#define EXT_MEMORY     //Enable external memory

//NOTES: IF YOU WANT TO PRINT DEBUG STRINGS UN-COMMENT NEXT LINE)
#define DEBUG //Enable debug print

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <FS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <time.h>
#include <Wire.h>               //defined but not used
#include <RTClib.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <string.h>
#include "pumpStruct.h"

//Select which include is correct
#ifdef EXT_MEMORY
  #include "Adafruit_FRAM_I2C.h"
#else
  #include <EEPROM.h>
  
  #define EEPROM_SIZE 4096
#endif

//WiFiManager, respect this order
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"        //https://github.com/tzapu/WiFiManager/archive/refs/tags/v2.0.15-rc.1.zip
#define WEBSERVER_H
#include "ESPAsyncWebSrv.h"

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

#define SDA_PIN 4
#define SCL_PIN 5

// Set PUMP GPIO
const int gpio_PUMP1 = 12;
const int gpio_PUMP2 = 13;
const int gpio_PUMP3 = 14;
const int gpio_PUMP4 = 15;
const int pinReset = 16;
const int pinButton = 0;

pumpStruct pump1;
pumpStruct pump2;
pumpStruct pump3;
pumpStruct pump4;

pumpStruct tempholder;
Settings settings;
Settings tempSettings;
// Init ESP8266 timer 0
ESP8266Timer ITimer;
RTC_DS3231 rtc;

#define HW_TIMER_INTERVAL_MS        1000

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
volatile unsigned long epochTime_Estimate;
unsigned long epochTime_Truth;
volatile unsigned int ticks = 0;
volatile unsigned int sync_ticks = 21600;
volatile unsigned int rtc_sync_ticks = 21600;

unsigned long startMillis = 0;        // will store last time LED was updated
unsigned long stopMillis = 0;


const char* PARAM_DST = "DST";
const char* PARAM_CALVOLUME = "calibrationVolume";
const char* PARAM_TIMEZONEOFFSET = "timezoneOffset";

const char* PARAM_PUMP1CONTAINERVOLUME = "pump1ContainerVolume";
const char* PARAM_PUMP1DATETIME = "pump1DateTime";
const char* PARAM_PUMP1DAYDELAY = "pump1DayDelay";
const char* PARAM_PUMP1AMOUNT = "pump1DispensingAmount";
const char* PARAM_PUMP1DAY = "pump1day";
const char* PARAM_PUMP1ENABLE = "pump1ProgStatus";

const char* PARAM_PUMP2CONTAINERVOLUME = "pump2ContainerVolume";
const char* PARAM_PUMP2DATETIME = "pump2DateTime";
const char* PARAM_PUMP2DAYDELAY = "pump2DayDelay";
const char* PARAM_PUMP2AMOUNT = "pump2DispensingAmount";
const char* PARAM_PUMP2DAY = "pump2day";
const char* PARAM_PUMP2ENABLE = "pump2ProgStatus";

const char* PARAM_PUMP3CONTAINERVOLUME = "pump3ContainerVolume";
const char* PARAM_PUMP3DATETIME = "pump3DateTime";
const char* PARAM_PUMP3DAYDELAY = "pump3DayDelay";
const char* PARAM_PUMP3AMOUNT = "pump3DispensingAmount";
const char* PARAM_PUMP3DAY = "pump3day";
const char* PARAM_PUMP3ENABLE = "pump3ProgStatus";

const char* PARAM_PUMP4CONTAINERVOLUME = "pump4ContainerVolume";
const char* PARAM_PUMP4DATETIME = "pump4DateTime";
const char* PARAM_PUMP4DAYDELAY = "pump4DayDelay";
const char* PARAM_PUMP4AMOUNT = "pump4DispensingAmount";
const char* PARAM_PUMP4DAY = "pump4day";
const char* PARAM_PUMP4ENABLE = "pump4ProgStatus";

char buffer[40];
boolean commitSucc=false;
boolean rtcWorking=false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
#ifdef EXT_MEMORY
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();
#else
//Nothing?
#endif

AsyncEventSource events("/events");

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void ICACHE_RAM_ATTR TimerHandler()
{
  epochTime_Estimate = epochTime_Estimate + 1;
  ticks = ticks + 1;
  sync_ticks = sync_ticks +1;
  rtc_sync_ticks = rtc_sync_ticks +1;
}



void initRTC()
{
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC, proceed with internal clock");
    Serial.flush();
    rtcWorking = false;
//    abort();
  }
  else
  {
    rtcWorking = true;
  }

  if ((rtc.lostPower())&&(rtcWorking == true)) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

// Replaces placeholder with stored values
String processor(const String& var)
{
  if (var == PARAM_CALVOLUME)
  {
    return String(settings.calibrationVolumeL);
  }
  else if (var == PARAM_TIMEZONEOFFSET)
  {
    return String(settings.timezoneOffset / 3600);
  }
  else if (var == PARAM_PUMP1CONTAINERVOLUME)
  {
    return String(pump1.containerVolume);
  }
  else if (var == PARAM_PUMP1AMOUNT)
  {
    return String(pump1.volumePump_mL);
  }
  else if (var == PARAM_PUMP1DAYDELAY)
  {
    return String(pump1.dayDelay);
  }
  else if (var == PARAM_PUMP1DATETIME)
  {
    if(pump1.nextRuntime == 0)
    {
      return String("NA");
    }
    else
    {
      DateTime dt (pump1.nextRuntime+settings.timezoneOffset+(3600*settings.DST));
      char strBuf[50];
      if (dt.isPM())
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i PM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      else
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i AM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      return String(strBuf);
    }    
  }
  else if (var == PARAM_PUMP1DAY)
  {
    return String(pump1.dayDelay);
  }
  else if (var == PARAM_PUMP1ENABLE) {
    if (pump1.programEnable)
    {
      return String("Enabled");
    }
    else
    {
      return String("Disabled");
    }
  }
  else if (var == PARAM_PUMP2CONTAINERVOLUME)
  {
    return String(pump2.containerVolume);
  }
  else if (var == PARAM_PUMP2AMOUNT)
  {
    return String(pump2.volumePump_mL);
  }
  else if (var == PARAM_PUMP2DAYDELAY)
  {
    return String(pump2.dayDelay);
  }
  else if (var == PARAM_PUMP2DATETIME)
  {
    if(pump2.nextRuntime == 0)
    {
      return String("NA");
    }
    else
    {
      DateTime dt (pump2.nextRuntime+settings.timezoneOffset+(3600*settings.DST));
      char strBuf[50];
      if (dt.isPM())
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i PM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      else
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i AM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      return String(strBuf);
    }    
  }
  else if (var == PARAM_PUMP2DAY)
  {
    return String(pump2.dayDelay);
  }
  else if (var == PARAM_PUMP2ENABLE) {
    if (pump2.programEnable)
    {
      return String("Enabled");
    }
    else
    {
      return String("Disabled");
    }
  }
  else if (var == PARAM_PUMP3CONTAINERVOLUME)
  {
    return String(pump3.containerVolume);
  }
  else if (var == PARAM_PUMP3AMOUNT)
  {
    return String(pump3.volumePump_mL);
  }
  else if (var == PARAM_PUMP3DAYDELAY)
  {
    return String(pump3.dayDelay);
  }
  else if (var == PARAM_PUMP3DATETIME)
  {
    if(pump3.nextRuntime == 0)
    {
      return String("NA");
    }
    else
    {
      DateTime dt (pump3.nextRuntime+settings.timezoneOffset+(3600*settings.DST));
      char strBuf[50];
      if (dt.isPM())
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i PM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      else
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i AM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      return String(strBuf);
    }    
  }
  else if (var == PARAM_PUMP3DAY)
  {
    return String(pump3.dayDelay);
  }
  else if (var == PARAM_PUMP3ENABLE) {
    if (pump3.programEnable)
    {
      return String("Enabled");
    }
    else
    {
      return String("Disabled");
    }
  }
  else if (var == PARAM_PUMP4CONTAINERVOLUME)
  {
    return String(pump4.containerVolume);
  }
  else if (var == PARAM_PUMP4AMOUNT)
  {
    return String(pump4.volumePump_mL);
  }
  else if (var == PARAM_PUMP4DAYDELAY)
  {
    return String(pump4.dayDelay);
  }
  else if (var == PARAM_PUMP4DATETIME)
  {
    if(pump4.nextRuntime == 0)
    {
      return String("NA");
    }
    else
    {
      DateTime dt (pump4.nextRuntime+settings.timezoneOffset+(3600*settings.DST));
      char strBuf[50];
      if (dt.isPM())
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i PM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      else
      {
       sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i AM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
      }
      return String(strBuf);
    }    
  }
  else if (var == PARAM_PUMP4DAY)
  {
    return String(pump4.dayDelay);
  }
  else if (var == PARAM_PUMP4ENABLE) {
    if (pump1.programEnable)
    {
      return String("Enabled");
    }
    else
    {
      return String("Disabled");
    }
  }
  else
  {
    return String();
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("Initialize System");
  initRTC();
  #ifdef EXT_MEMORY
    if (fram.begin(0x50))
    {  // you can stick the new i2c addr in here, e.g. begin(0x51);
     Serial.println("Found I2C FRAM");
    }
    else
    {
     Serial.println("I2C FRAM not identified ... check your connections?\r\n");
     Serial.println("Will continue in case this processor doesn't support repeated start\r\n");
    }
  #else
    pinMode(SDA_PIN,OUTPUT);
    pinMode(SCL_PIN,OUTPUT);
    EEPROM.begin(EEPROM_SIZE);
  #endif

  pinMode(gpio_PUMP1, OUTPUT);
  digitalWrite(gpio_PUMP1, LOW);
  pinMode(gpio_PUMP2, OUTPUT);
  digitalWrite(gpio_PUMP2, LOW);
  pinMode(gpio_PUMP3, OUTPUT);
  digitalWrite(gpio_PUMP3, LOW);
  pinMode(gpio_PUMP4, OUTPUT);
  digitalWrite(gpio_PUMP4, LOW);  

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  WiFi.hostname("MicroDoser");

  if(!wifiManager.autoConnect("MicroDoser")) {
    Serial.println("Connection Timout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  Serial.println(F("WIFIManager connected!"));

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
   Serial.println("An Error has occurred while mounting SPIFFS");
   return;
  }  
  
  timeClient.begin();
  DateTime now_Time;
  if(rtcWorking == true)
  {
    DateTime now_Time = rtc.now(); 
  }
  else
  {
     //TODO: rtc fail implementantion
  }
  epochTime_Truth = now_Time.unixtime();
  
 // Serial.println(epochTime_Truth);
  
  epochTime_Estimate = epochTime_Truth;

// Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(millis());
  }
  else
  {
    Serial.println(F("Can't set ITimer correctly. Select another frequency or interval"));
  }
  pump1.valid_read = false; //                                                                     1byte
  pump1.CAL_pumpmS=0; //time in mSecs to run pump 100 mL of fluid                                   2bytes
  pump1.previousRuntime=0; //last time the pump ran in Epoch time (S)                     4bytes
  pump1.nextRuntime=0; //next time the pump needs to run in Epoch time (S)                4bytes
  pump1.dayDelay=10; //how many days until the pump runs again                                      2bytes
  pump1.volumePump_mL = 1;  //                                                                    2bytes
  pump1.totalvolumePumped_mL=0; //running total of all the liquid the pump has pumped (mL)        2bytes
  pump1.containerVolume=0; //amount of in pump reservoir.                                         2bytes
  pump1.motor_GPIO = 2;//                                                                           2bytes
  pump1.programEnable = false;//                                                                   1byte
  pump1.page = 0; //

  EEPROM.put(100, pump1);
  EEPROM.put(200, "DI");
  EEPROM.put(300, "SCRITTURA");
  EEPROM.put(400, "E LETTURA");
  #ifdef EXT_MEMORY
    fram.read((uint16_t)pump1.page, tempholder);
    fram.read((uint16_t)pump2.page, tempholder);
    fram.read((uint16_t)pump3.page, tempholder);
    fram.read((uint16_t)pump4.page, tempholder);
  #else
    EEPROM.get(100, tempholder);
    #ifdef DEBUG
      //memcpy(buffer, &tempholder.motor_GPIO, sizeof(buffer));
      itoa(tempholder.motor_GPIO, buffer, 10);
      Serial.println("Values read on P1 setup are: ");
      Serial.println(buffer);
    #endif    
    EEPROM.get(200, buffer);
    #ifdef DEBUG
      //memcpy(buffer, &pump2, sizeof(buffer));
      Serial.println("Values read on P2 setup are: ");
      Serial.println(buffer);
    #endif
    EEPROM.get(300, buffer);
    #ifdef DEBUG
      //memcpy(buffer, &pump3, sizeof(buffer));
      Serial.println("Values read on P3 setup are: ");
      Serial.println(buffer);
    #endif
    EEPROM.get(400, buffer);
    #ifdef DEBUG
      //memcpy(buffer, &pump4, sizeof(buffer));
      Serial.println("Values read on P4 setup are: ");
      Serial.println(buffer);
    #endif
  #endif
  
  if (tempholder.valid_read)
  {
    pump1 = tempholder;
    tempholder.valid_read = false;
  }
  else
  {
    pump1.valid_read = true;
  }
  

  #ifdef EXT_MEMORY
    fram.read((uint16_t)settings.page, tempSettings);
  #else
    EEPROM.get((uint16_t)settings.page,tempSettings);
  #endif

  //Hard set parameters that might get changed
  pump1.motor_GPIO = gpio_PUMP1;
  pump2.motor_GPIO = gpio_PUMP2;
  pump3.motor_GPIO = gpio_PUMP3;
  pump4.motor_GPIO = gpio_PUMP4;

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/javascript.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/javascript.js", "text/javascript");
  });

  // Receive an HTTP GET request
  server.on("/cal1on1", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump1.programEnable = false;
    digitalWrite(pump1.motor_GPIO, HIGH);
    startMillis = millis();
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1on2", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump2.programEnable = false;
    digitalWrite(pump2.motor_GPIO, HIGH);
    startMillis = millis();
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1on3", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump3.programEnable = false;
    digitalWrite(pump3.motor_GPIO, HIGH);
    startMillis = millis();
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1on4", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump4.programEnable = false;
    digitalWrite(pump4.motor_GPIO, HIGH);
    startMillis = millis();
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/cal1off1", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump1.programEnable = true;
    digitalWrite(pump1.motor_GPIO, LOW);
    stopMillis = millis();
    pump1.CAL_pumpmS = (stopMillis - startMillis);
    Serial.println(pump1.CAL_pumpmS);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1off2", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump2.programEnable = true;
    digitalWrite(pump2.motor_GPIO, LOW);
    stopMillis = millis();
    pump2.CAL_pumpmS = (stopMillis - startMillis);
    Serial.println(pump2.CAL_pumpmS);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1off3", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump3.programEnable = true;
    digitalWrite(pump3.motor_GPIO, LOW);
    stopMillis = millis();
    pump3.CAL_pumpmS = (stopMillis - startMillis);
    Serial.println(pump3.CAL_pumpmS);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/cal1off4", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump4.programEnable = true;
    digitalWrite(pump4.motor_GPIO, LOW);
    stopMillis = millis();
    pump4.CAL_pumpmS = (stopMillis - startMillis);
    Serial.println(pump4.CAL_pumpmS);
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/save1", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)pump1.page, pump1);
    #else
      EEPROM.put((uint16_t)pump1.page, pump1);
      commitSucc = EEPROM.commit();
      Serial.println((commitSucc) ? "Pump 01 parameters saved as requested" : "Commit Pump 01 parameters failed");
    #endif   
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/save2", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)pump2.page, pump2);
    #else
      EEPROM.put((uint16_t)pump2.page, pump2);
      commitSucc = EEPROM.commit();
      Serial.println((commitSucc) ? "Pump 02 parameters saved as requested" : "Commit Pump 02 parameters failed");
    #endif   
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/save3", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)pump3.page, pump3);
    #else
      EEPROM.put((uint16_t)pump3.page, pump3);
      commitSucc = EEPROM.commit();
      Serial.println((commitSucc) ? "Pump 03 parameters saved as requested" : "Commit Pump 03 parameters failed");
    #endif   
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/save4", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)pump4.page, pump4);
    #else
      EEPROM.put((uint16_t)pump4.page, pump4);
      commitSucc = EEPROM.commit();
      Serial.println((commitSucc) ? "Pump 04 parameters saved as requested" : "Commit Pump 04 parameters failed");
    #endif   
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/pump1runtime", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    request->send_P(200, "text/plain", String(pump1.CAL_pumpmS / 1000.0).c_str());
  });
  server.on("/pump2runtime", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    request->send_P(200, "text/plain", String(pump2.CAL_pumpmS / 1000.0).c_str());
  });
  server.on("/pump3runtime", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    request->send_P(200, "text/plain", String(pump3.CAL_pumpmS / 1000.0).c_str());
  });
  server.on("/pump4runtime", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    request->send_P(200, "text/plain", String(pump4.CAL_pumpmS / 1000.0).c_str());
  });


  server.on("/on1", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump1.programEnable = false;
    digitalWrite(pump1.motor_GPIO, HIGH);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/on2", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump2.programEnable = false;
    digitalWrite(pump2.motor_GPIO, HIGH);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/on3", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump3.programEnable = false;
    digitalWrite(pump3.motor_GPIO, HIGH);
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/on4", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    pump4.programEnable = false;
    digitalWrite(pump4.motor_GPIO, HIGH);
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/off1", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    digitalWrite(pump1.motor_GPIO, LOW);
    pump1.programEnable = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/off2", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    digitalWrite(pump2.motor_GPIO, LOW);
    pump2.programEnable = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/off3", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    digitalWrite(pump3.motor_GPIO, LOW);
    pump3.programEnable = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/off4", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    digitalWrite(pump4.motor_GPIO, LOW);
    pump4.programEnable = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/getSetting", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    String temp;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_PUMP1CONTAINERVOLUME)) 
    {
      temp = request->getParam(PARAM_PUMP1CONTAINERVOLUME)->value();
      if (temp.length() == 0)
      {      
        Serial.println("Empty String");
      }        
      else
      {
        pump1.containerVolume = temp.toFloat();
      }
      //TODO: Add management for other 3 pumps
    }
    
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    if (request->hasParam(PARAM_TIMEZONEOFFSET))
    {
      temp = request->getParam(PARAM_TIMEZONEOFFSET)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        settings.timezoneOffset  = 3600 * temp.toInt();
      }
    }

    if (request->hasParam(PARAM_DST))
    {
      temp = request->getParam(PARAM_DST)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }        
      else
      {
        settings.DST = temp.toInt();
      }
    }
    else
    {
    }
    
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)settings.page, settings);
      fram.write((uint16_t)pump1.page, pump1);
    #else
      EEPROM.put((uint16_t)settings.page, settings);
      commitSucc = EEPROM.commit();
      EEPROM.put((uint16_t)pump1.page, pump1);
      commitSucc = EEPROM.commit();
    #endif
    #ifdef DEBUG
      memcpy(buffer, &settings, sizeof(buffer));
      Serial.println("settings DST ");
      Serial.println(buffer);
      memcpy(buffer, &pump1, sizeof(buffer));
      Serial.println("P1 DST ");
      Serial.println(buffer);
    #endif

    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/getProgram", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_PUMP1DATETIME)) {
      String temp = request->getParam(PARAM_PUMP1DATETIME)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        temp += ":00";
        #ifdef DEBUG
          Serial.println("Request for time/date ");
          Serial.println(temp.c_str());
        #endif

        struct tm tm;
        time_t t;
        if (strptime(temp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
        {
          /* Handle error */;
          tm.tm_isdst = 0;      /* Not set by strptime(); tells mktime()to determine whether daylight saving time is in effect */
        }
        t = mktime(&tm);
        t = t - settings.timezoneOffset - (3600*settings.DST);
        if (t == -1)
        {
          /* Handle error */;
          printf("%ld\n", (long) t);
          printf("%ld\n", epochTime_Estimate);
          pump1.nextRuntime = (long) t;
          #ifdef EXT_MEMORY
            fram.write((uint16_t)pump1.page, pump1);
          #else
            EEPROM.put((uint16_t)pump1.page, pump1);
            commitSucc = EEPROM.commit();
          #endif        
          #ifdef DEBUG
            memcpy(buffer, &pump1, sizeof(buffer));
            Serial.println("P1 next time run ");
            Serial.println(buffer);
          #endif
        }
      }
    }
    if (request->hasParam(PARAM_PUMP1AMOUNT)) {
      String temp = request->getParam(PARAM_PUMP1AMOUNT)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump1.volumePump_mL = temp.toFloat();
        #ifdef EXT_MEMORY        
          fram.write((uint16_t)pump1.page, pump1);
        #else
          EEPROM.put((uint16_t)pump1.page, pump1);
          commitSucc = EEPROM.commit();
        #endif        
        #ifdef DEBUG
          memcpy(buffer, &pump1, sizeof(buffer));
          Serial.println("P1 volume mL ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP1DAY)) {
      String temp = request->getParam(PARAM_PUMP1DAY)->value();

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump1.dayDelay = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump1.page, pump1);
        #else
          EEPROM.put((uint16_t)pump1.page, pump1);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump1, sizeof(buffer));
          Serial.println("P1 dayDelay ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP1ENABLE)) {
      String temp = request->getParam(PARAM_PUMP1ENABLE)->value();
      #ifdef DEBUG
        Serial.println("P1 dosing date");
        Serial.println(temp);
      #endif

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump1.programEnable = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump1.page, pump1);
        #else
          EEPROM.put((uint16_t)pump1.page, pump1);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump1, sizeof(buffer));
          Serial.println("P1 enable ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP2DATETIME)) {
      String temp = request->getParam(PARAM_PUMP2DATETIME)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        temp += ":00";
        Serial.println(temp.c_str());
        //
        struct tm tm;
        time_t t;
        if (strptime(temp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
        {
          /* Handle error */;
          tm.tm_isdst = 0;      /* Not set by strptime(); tells mktime()to determine whether daylight saving time is in effect */
        }
        t = mktime(&tm);
        t = t - settings.timezoneOffset - (3600*settings.DST);
        if (t == -1)
        {
          /* Handle error */;
          printf("%ld\n", (long) t);
          printf("%ld\n", epochTime_Estimate);
          pump2.nextRuntime = (long) t;
          #ifdef EXT_MEMORY
            fram.write((uint16_t)pump2.page, pump2);
          #else
            EEPROM.put((uint16_t)pump2.page, pump2);
            commitSucc = EEPROM.commit();
          #endif        
          #ifdef DEBUG
          memcpy(buffer, &pump2, sizeof(buffer));
          Serial.println("P2 nextRuntime ");
          Serial.println(buffer);
          #endif
        }
      }
    }
    if (request->hasParam(PARAM_PUMP2AMOUNT)) {
      String temp = request->getParam(PARAM_PUMP2AMOUNT)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump2.volumePump_mL = temp.toFloat();
        #ifdef EXT_MEMORY        
          fram.write((uint16_t)pump2.page, pump2);
        #else
          EEPROM.put((uint16_t)pump2.page, pump2);
          commitSucc = EEPROM.commit();
        #endif        
        #ifdef DEBUG
          memcpy(buffer, &pump2, sizeof(buffer));
          Serial.println("P2 volume pump ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP2DAY)) {
      String temp = request->getParam(PARAM_PUMP2DAY)->value();

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump2.dayDelay = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump2.page, pump2);
        #else
          EEPROM.put((uint16_t)pump2.page, pump2);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump2, sizeof(buffer));
          Serial.println("P2 dayDelay ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP2ENABLE)) {
      String temp = request->getParam(PARAM_PUMP2ENABLE)->value();
      Serial.println(temp);

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump2.programEnable = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump2.page, pump2);
        #else
          EEPROM.put((uint16_t)pump2.page, pump2);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump2, sizeof(buffer));
          Serial.println("P2 enable ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP3DATETIME)) {
      String temp = request->getParam(PARAM_PUMP3DATETIME)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        temp += ":00";
        Serial.println(temp.c_str());
        //
        struct tm tm;
        time_t t;
        if (strptime(temp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
        {
          /* Handle error */;
          tm.tm_isdst = 0;      /* Not set by strptime(); tells mktime()to determine whether daylight saving time is in effect */
        }
        t = mktime(&tm);
        t = t - settings.timezoneOffset - (3600*settings.DST);
        if (t == -1)
        {
          /* Handle error */;
          printf("%ld\n", (long) t);
          printf("%ld\n", epochTime_Estimate);
          pump3.nextRuntime = (long) t;
          #ifdef EXT_MEMORY
            fram.write((uint16_t)pump3.page, pump3);
          #else
            EEPROM.put((uint16_t)pump3.page, pump3);
            commitSucc = EEPROM.commit();
          #endif        
          #ifdef DEBUG
            memcpy(buffer, &pump3, sizeof(buffer));
            Serial.println("P3 next time run ");
            Serial.println(buffer);
          #endif
        }
      }
    }
    if (request->hasParam(PARAM_PUMP3AMOUNT)) {
      String temp = request->getParam(PARAM_PUMP3AMOUNT)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump3.volumePump_mL = temp.toFloat();
        #ifdef EXT_MEMORY        
          fram.write((uint16_t)pump3.page, pump3);
        #else
          EEPROM.put((uint16_t)pump3.page, pump3);
          commitSucc = EEPROM.commit();
        #endif        
        #ifdef DEBUG
          memcpy(buffer, &pump3, sizeof(buffer));
          Serial.println("P3 pump volume ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP3DAY)) {
      String temp = request->getParam(PARAM_PUMP3DAY)->value();

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump3.dayDelay = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump3.page, pump3);
        #else
          EEPROM.put((uint16_t)pump3.page, pump3);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump3, sizeof(buffer));
          Serial.println("P3 dayDelay ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP3ENABLE)) {
      String temp = request->getParam(PARAM_PUMP3ENABLE)->value();
      Serial.println(temp);

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump3.programEnable = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump3.page, pump3);
        #else
          EEPROM.put((uint16_t)pump3.page, pump3);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
          memcpy(buffer, &pump3, sizeof(buffer));
          Serial.println("P3 enable ");
          Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP4DATETIME)) {
      String temp = request->getParam(PARAM_PUMP4DATETIME)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        temp += ":00";
        Serial.println(temp.c_str());
        //
        struct tm tm;
        time_t t;
        if (strptime(temp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
        {
          /* Handle error */;
          tm.tm_isdst = 0;      /* Not set by strptime(); tells mktime()to determine whether daylight saving time is in effect */
        }
        t = mktime(&tm);
        t = t - settings.timezoneOffset - (3600*settings.DST);
        if (t == -1)
        {
          /* Handle error */;
          printf("%ld\n", (long) t);
          printf("%ld\n", epochTime_Estimate);
          pump4.nextRuntime = (long) t;
          #ifdef EXT_MEMORY
            fram.write((uint16_t)pump4.page, pump4);
          #else
            EEPROM.put((uint16_t)pump4.page, pump4);
            commitSucc = EEPROM.commit();
          #endif        
          #ifdef DEBUG
            memcpy(buffer, &pump4, sizeof(buffer));
            Serial.println("P5 next time run ");
            Serial.println(buffer);
          #endif
        }
      }
    }
    if (request->hasParam(PARAM_PUMP4AMOUNT)) {
      String temp = request->getParam(PARAM_PUMP4AMOUNT)->value();
      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump4.volumePump_mL = temp.toFloat();
        #ifdef EXT_MEMORY        
          fram.write((uint16_t)pump4.page, pump4);
        #else
          EEPROM.put((uint16_t)pump4.page, pump4);
          commitSucc = EEPROM.commit();
        #endif        
        #ifdef DEBUG
            memcpy(buffer, &pump4, sizeof(buffer));
            Serial.println("P4 volume pump ");
            Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP4DAY)) {
      String temp = request->getParam(PARAM_PUMP4DAY)->value();

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump4.dayDelay = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump4.page, pump4);
        #else
          EEPROM.put((uint16_t)pump4.page, pump4);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
            memcpy(buffer, &pump4, sizeof(buffer));
            Serial.println("P4 dayDelay ");
            Serial.println(buffer);
        #endif
      }
    }
    if (request->hasParam(PARAM_PUMP4ENABLE)) {
      String temp = request->getParam(PARAM_PUMP4ENABLE)->value();
      Serial.println(temp);

      if (temp.length() == 0)
      {
        Serial.println("Empty String");
      }
      else
      {
        pump4.programEnable = temp.toInt();
        #ifdef EXT_MEMORY
          fram.write((uint16_t)pump4.page, pump4);
        #else
          EEPROM.put((uint16_t)pump1.page, pump4);
          commitSucc = EEPROM.commit();
        #endif
        #ifdef DEBUG
            memcpy(buffer, &pump4, sizeof(buffer));
            Serial.println("P4 enable ");
            Serial.println(buffer);
        #endif
      }
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

   // Handle Web Server Events
   events.onConnect([](AsyncEventSourceClient *client){
     if(client->lastId()){
       Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
     }
     // send event with message "hello!", id current millis
     // and set reconnect delay to 1 second
     client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.onNotFound(notFound);
  server.begin();
}

void factoryReset()
{
    if (false == digitalRead(pinButton))
    {
        Serial.println("Hold the button to reset to factory defaults...");
        bool cancel = false;
        for (int iter=0; iter<30; iter++)
        {
            digitalWrite(pinReset, HIGH);
            delay(100);
            if (true == digitalRead(pinButton))
            {
                cancel = true;
                break;
            }
            digitalWrite(pinReset, LOW);
            delay(100);
            if (true == digitalRead(pinButton))
            {
                cancel = true;
                break;
            }
        }
        if (false == digitalRead(pinButton) && !cancel)
        {
            digitalWrite(pinReset, HIGH);
            Serial.println("Disconnecting...");
            WiFi.disconnect();

            Serial.println("Restarting...");
            //WIP: Beed to clean stored values in eeprom, not format the whole fs.
            //SPIFFS.format();
            ESP.restart();
        }
        else
        {
            // Cancel reset to factory defaults
            Serial.println("Reset to factory defaults cancelled.");
            digitalWrite(pinReset, LOW);
        }
    }
}

void loop() {
  ArduinoOTA.handle();

  // Allow MDNS processing
  MDNS.update();

  if (ticks > 0)
  {
    events.send(String(epochTime_Estimate).c_str(),"currentTime",millis());
    ticks = 0;
  }
  if (sync_ticks > settings.time_sync_check)
  {
    sync_ticks = 0;
    DateTime now_Time;
    if(rtcWorking == true)
    {
      DateTime now_Time = rtc.now(); 
    }
    else
    {
       //TODO: rtc fail implementantion
    }
    epochTime_Estimate = now_Time.unixtime();     
  }
  if (rtc_sync_ticks > settings.rtc_time_sync_check)
  {
    rtc_sync_ticks = 0;
    unsigned long time_truth = getTime();
    if (time_truth > epochTime_Estimate-1000)
    {
      epochTime_Estimate = time_truth;      
      if(rtcWorking == true)
      {
         rtc.adjust(DateTime(time_truth));
      }
      else
      {
         //TODO: rtc fail implementantion
      }
    }   
  }

  checkPump(pump1, "1");
  checkPump(pump2, "2");
  checkPump(pump3, "3");
  checkPump(pump4, "4");

  //WIP: Implement Reset to Defaults
  factoryReset();
}

void checkPump(pumpStruct pump, const char pumpn[1])
{
  if (pump.nextRuntime  ==  epochTime_Estimate && pump.programEnable == true && pump.containerVolume > 0)
  {
    digitalWrite(pump.motor_GPIO, HIGH);
    pump.previousRuntime =  pump.nextRuntime;
    pump.nextRuntime = pump.nextRuntime + (pump.dayDelay) * 86400;
    pump.totalvolumePumped_mL += pump.volumePump_mL;
    pump.containerVolume -= pump.volumePump_mL;
    #ifdef EXT_MEMORY    
      fram.write((uint16_t)pump.page, pump);
    #else
      EEPROM.put((uint16_t)pump.page, pump);
      commitSucc = EEPROM.commit();
    #endif
    #ifdef DEBUG
      memcpy(buffer, &pump, sizeof(buffer));
      Serial.println("Pump page ");
      Serial.println(buffer);
    #endif
    delay(round((pump.volumePump_mL * pump.CAL_pumpmS) / settings.calibrationVolumeL));
    Serial.print(round((pump.volumePump_mL * pump.CAL_pumpmS) / settings.calibrationVolumeL));
    digitalWrite(pump.motor_GPIO, LOW);
    events.send(String(pump.nextRuntime).c_str(),"pumpnextDatetime",millis());
  }
  else if ((pump.nextRuntime  < epochTime_Estimate) && (pump.programEnable == true))
  {
    pump.nextRuntime = pump.nextRuntime + (86400*pump.dayDelay);
  }
  #ifdef DEBUG_1
      memcpy(buffer, &pump, sizeof(buffer));
      Serial.println("check pump ");
      Serial.println(pumpn);
      Serial.println(buffer);
  #endif
}
