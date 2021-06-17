#ifndef PUMPSTRUCT_H
#define PUMPSTRUCT_H

struct pumpStruct{
bool valid_read = false;
int CAL_pumpmS=0; //time in mSecs to run pump 100 mL of fluid
unsigned long previousRuntime=0; //last time the pump ran in Epoch time (S)
unsigned long nextRuntime=0; //next time the pump needs to run in Epoch time (S)
int dayDelay=10; //how many days until the pump runs again
float volumePump_mL = 1;
float totalvolumePumped_mL=0; //running total of all the liquid the pump has pumped (mL)
float containerVolume=0; //amount of in pump reservoir. 
int motor_GPIO = 2;
bool programEnable = false;
uint16_t page = 0;
};


struct Settings{
bool valid_read = false;
float calibrationVolumeL = 20.0;
int timezoneOffset = -18000;
bool DST = 0;
unsigned int time_sync_check = 600;
unsigned int rtc_time_sync_check = 21600;
uint16_t page = 16384;
};

#endif
