// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0


#define SDA_PIN 4
#define SCL_PIN 5

#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

#include "pumpStruct.h"

#include "Adafruit_FRAM_I2C.h"

// Replace with your network credentials
const char* ssid = "The Promise LAN";
const char* password = "Therecanonlybeone!!!!!!!!!!!!!!!!!!!";

// Set LED GPIO
const int gpio_PUMP1 = 12;

pumpStruct pump1;
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
volatile unsigned int sync_ticks = 1800;

unsigned long startMillis = 0;        // will store last time LED was updated
unsigned long stopMillis = 0;


const char* PARAM_CONTAINER1VOLUME = "pump1ContainerVolume";
const char* PARAM_CALVOLUME = "calibrationVolume";
const char* PARAM_TIMEZONEOFFSET = "timezoneOffset";
const char* PARAM_PUMP1DATETIME = "pump1DateTime";
const char* PARAM_PUMP1DAYDELAY = "pump1DayDelay";
const char* PARAM_DST = "DST";
const char* PARAM_PUMP1AMOUNT = "pump1DispensingAmount";
const char* PARAM_PUMP1DAY = "pump1day";
const char* PARAM_PUMP1ENABLE = "pump1ProgStatus";


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();

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
}



void initRTC()
{
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
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
String processor(const String& var) {

  if (var == "pump1ContainerVolume") {
    return String(pump1.containerVolume);
  }

  else if (var == "calibrationVolume") {
    return String(settings.calibrationVolumeL);
  }

  else if (var == "timezoneOffset") {

    return String(settings.timezoneOffset / 3600);
  }

  else if (var == "pump1DispensingAmount") {

    return String(pump1.volumePump_mL);
  }

   else if (var == "pump1ProgTime") {

    if(pump1.nextRuntime == 0)
    return String("NA");

    else
    {
      DateTime dt (pump1.nextRuntime+settings.timezoneOffset+(3600*settings.DST));
  char strBuf[50];
  if (dt.isPM())
  sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i PM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
     else
      sprintf(strBuf, "%02i/%02i/%i, %02i:%02i:%02i AM", dt.month(), dt.day(), dt.year(), dt.twelveHour(), dt.minute(), dt.second() );
    return String(strBuf);
    }

    
  }
  else if (var == "pump1day") {

    return String(pump1.dayDelay);
  }
  else if (var == "pump1ProgStatus") {

    if (pump1.programEnable)
      return String("Enabled");
    else
      return String("Disabled");
  }
  else
    return String();
}



void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  initRTC();

   if (fram.begin(0x50)) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
  } else {
    Serial.println("I2C FRAM not identified ... check your connections?\r\n");
    Serial.println("Will continue in case this processor doesn't support repeated start\r\n");
  }


  pinMode(gpio_PUMP1, OUTPUT);
  digitalWrite(gpio_PUMP1, LOW);

   WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
    }  

  
  timeClient.begin();
  DateTime now_Time = rtc.now(); 
  epochTime_Truth = now_Time.unixtime();
  
 // Serial.println(epochTime_Truth);
  
  epochTime_Estimate = epochTime_Truth;



  
// Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval"));


fram.read((uint16_t)pump1.page, tempholder);
Serial.println("Fram Read is:");
Serial.println(tempholder.valid_read);
if (tempholder.valid_read)
{
pump1 = tempholder;
tempholder.valid_read = false;
}

else
pump1.valid_read = true;



fram.read((uint16_t)settings.page, tempSettings);

if (tempSettings.valid_read)
{
settings = tempSettings;
tempSettings.valid_read = false;
}
else
settings.valid_read = true;


//Hard set parameters that might get changed
 pump1.motor_GPIO = gpio_PUMP1;

  //

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/javascript.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/javascript.js", "text/javascript");
  });

// Receive an HTTP GET request
  server.on("/cal1on", HTTP_GET, [] (AsyncWebServerRequest * request) {
    pump1.programEnable = false;
    digitalWrite(pump1.motor_GPIO, HIGH);
    startMillis = millis();
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // Receive an HTTP GET request
  server.on("/cal1off", HTTP_GET, [] (AsyncWebServerRequest * request) {
    pump1.programEnable = true;
    digitalWrite(pump1.motor_GPIO, LOW);
    stopMillis = millis();
    pump1.CAL_pumpmS = (stopMillis - startMillis);
    Serial.println(pump1.CAL_pumpmS);
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/save1", HTTP_GET, [] (AsyncWebServerRequest * request) {
   fram.write((uint16_t)pump1.page, pump1);
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/pump1runtime", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(pump1.CAL_pumpmS / 1000.0).c_str());
  });


  // Receive an HTTP GET request
  server.on("/on1", HTTP_GET, [] (AsyncWebServerRequest * request) {
    pump1.programEnable = false;
    digitalWrite(pump1.motor_GPIO, HIGH);
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // Receive an HTTP GET request
  server.on("/off1", HTTP_GET, [] (AsyncWebServerRequest * request) {
    digitalWrite(pump1.motor_GPIO, LOW);
    pump1.programEnable = true;
    request->send(SPIFFS, "/index.html", String(), false);
  });


  server.on("/getSetting", HTTP_GET, [] (AsyncWebServerRequest * request) {

    String temp;

    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_CONTAINER1VOLUME)) {

      temp = request->getParam(PARAM_CONTAINER1VOLUME)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");


      else
        pump1.containerVolume = temp.toFloat();



    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    if (request->hasParam(PARAM_TIMEZONEOFFSET)) {
      temp = request->getParam(PARAM_TIMEZONEOFFSET)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");


      else
        settings.timezoneOffset  = 3600 * temp.toInt();



    }

    if (request->hasParam(PARAM_DST)) {
      temp = request->getParam(PARAM_DST)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");


      else
        settings.DST = temp.toInt();

    }


    else {

    }
 fram.write((uint16_t)settings.page, settings);

 fram.write((uint16_t)pump1.page, pump1);


    request->send(SPIFFS, "/index.html", String(), false, processor);
  });




  server.on("/getProgram", HTTP_GET, [] (AsyncWebServerRequest * request) {

    if (request->hasParam(PARAM_PUMP1DATETIME)) {
      String temp = request->getParam(PARAM_PUMP1DATETIME)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");

      else
      {
        temp += ":00";
        Serial.println(temp.c_str());
        //
        struct tm tm;
        time_t t;


        if (strptime(temp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL)
          /* Handle error */;


        tm.tm_isdst = 0;      /* Not set by strptime(); tells mktime()
                          to determine whether daylight saving time
                          is in effect */

  
        t = mktime(&tm);
        t = t - settings.timezoneOffset - (3600*settings.DST);
        if (t == -1)
          /* Handle error */;
        printf("%ld\n", (long) t);
        printf("%ld\n", epochTime_Estimate);
        pump1.nextRuntime = (long) t;
 fram.write((uint16_t)pump1.page, pump1);
//
      }
    }
    if (request->hasParam(PARAM_PUMP1AMOUNT)) {
      String temp = request->getParam(PARAM_PUMP1AMOUNT)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");

      else
      {
        pump1.volumePump_mL = temp.toFloat();
 fram.write((uint16_t)pump1.page, pump1);
      }
    }



    if (request->hasParam(PARAM_PUMP1DAY)) {
      String temp = request->getParam(PARAM_PUMP1DAY)->value();

      if (temp.length() == 0)
        Serial.println("Empty String");

      else
      {
        pump1.dayDelay = temp.toInt();
 fram.write((uint16_t)pump1.page, pump1);
      }
    }

    if (request->hasParam(PARAM_PUMP1ENABLE)) {
      String temp = request->getParam(PARAM_PUMP1ENABLE)->value();
      Serial.println(temp);

      if (temp.length() == 0)
        Serial.println("Empty String");


      else
        pump1.programEnable = temp.toInt();
 fram.write((uint16_t)pump1.page, pump1);

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

void loop() {
ArduinoOTA.handle();

  if (ticks > 0)
  {
    events.send(String(epochTime_Estimate).c_str(),"currentTime",millis());
    ticks = 0;
  }

  if (sync_ticks > settings.time_sync_check)
  {
    sync_ticks = 0;
    unsigned long time_truth = getTime();

    if (time_truth > 0)
    {
      epochTime_Estimate = time_truth;      
      rtc.adjust(DateTime(time_truth));
    }

    else
    {
      DateTime now_Time = rtc.now(); 
      epochTime_Estimate = now_Time.unixtime();      
    }
    
    
  }

//Serial.println(pump1.nextRuntime);
//Serial.println(epochTime_Estimate);
  if (pump1.nextRuntime  ==  epochTime_Estimate && pump1.programEnable == true && pump1.containerVolume > 0)
  {
    digitalWrite(pump1.motor_GPIO, HIGH);
    pump1.previousRuntime =  pump1.nextRuntime;
    pump1.nextRuntime = pump1.nextRuntime + (pump1.dayDelay) * 86400;
    pump1.totalvolumePumped_mL += pump1.volumePump_mL;
    pump1.containerVolume -= pump1.volumePump_mL;
 fram.write((uint16_t)pump1.page, pump1);
    delay(round((pump1.volumePump_mL * pump1.CAL_pumpmS) / settings.calibrationVolumeL));
    Serial.print(round((pump1.volumePump_mL * pump1.CAL_pumpmS) / settings.calibrationVolumeL));
    digitalWrite(pump1.motor_GPIO, LOW);
    events.send(String(pump1.nextRuntime).c_str(),"pump1nextDatetime",millis());
    
  }

  else if ((pump1.nextRuntime  < epochTime_Estimate) && (pump1.programEnable == true))
  {
    
    pump1.nextRuntime = pump1.nextRuntime + (86400*pump1.dayDelay);
 fram.write((uint16_t)pump1.page, pump1);
  }



}
