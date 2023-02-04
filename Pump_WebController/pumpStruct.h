#ifndef PUMPSTRUCT_H
#define PUMPSTRUCT_H

struct pumpStruct{
  bool valid_read = false; //                                                                     1byte
  int CAL_pumpmS=0; //time in mSecs to run pump 100 mL of fluid                                   2bytes
  unsigned long previousRuntime=0; //last time the pump ran in Epoch time (S)                     4bytes
  unsigned long nextRuntime=0; //next time the pump needs to run in Epoch time (S)                4bytes
  int dayDelay=10; //how many days until the pump runs again                                      2bytes
  float volumePump_mL = 1;  //                                                                    2bytes
  float totalvolumePumped_mL=0; //running total of all the liquid the pump has pumped (mL)        2bytes
  float containerVolume=0; //amount of in pump reservoir.                                         2bytes
  int motor_GPIO = 2;//                                                                           2bytes
  bool programEnable = false;//                                                                   1byte
  uint16_t page = 0; //                                                                           2bytes
}; //                                                                 total size needed in rom   24bytes


struct Settings{
  bool valid_read = false;//                                                                      1byte
  float calibrationVolumeL = 20.0;//                                                              2bytes
  int timezoneOffset = -18000;//                                                                  2bytes
  bool DST = 0;//                                                                                 1byte
  unsigned int time_sync_check = 600;//                                                           2bytes
  unsigned int rtc_time_sync_check = 21600;//                                                     2bytes
  uint16_t page = 16384;//                                                                        2bytes
};//                                                                   total size needed in rom   12bytes

#endif
