# Aquarium-Microdosing-Pump-Controller
Software for an ESP8266 base aquarium micro-dosing pump. Controlled through a local webpage. Currently hard-coded to control a single pump, the hardware supports up to four. 

Required Libraries:
Adafruit I2C FRAM Library
Adafruit RTClib
ESPAsyncWebServer
ESPAsyncTCP
NTPClient
ESP8266TimerInterrupt

Also needed:
Setting up Arduino IDE to support uploading files to SPIFFS: https://randomnerdtutorials.com/install-esp8266-filesystem-uploader-arduino-ide/

Required Hardware:
MB85RC256V I2C FRAM. This is for saving configurations without constantly writing to internal memory to prevent excessive wearing. 
DS3231 RTC. For keeping track of time if wifi is unavailable. 

Known Problems:

Need to redesign PCB to be smaller and cheaper. Software is hard-coded for a single pump. 
