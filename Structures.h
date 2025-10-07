/* 
_sBoatData from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).

*/
#ifndef _Structures_H_
#define _Structures_H_
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h> // for the doubleNA

struct _sDisplay_Config {  // will be Display_Config for the JSON set Defaults and user settings for displays 
  char Mag_Var[15]; // got to save double variable as a string! east is positive
  int Start_Page ;
  int LocalTimeOffset;
  char PanelName[25];
  char APpassword[25]; 
  char FourWayBR[10] ;
  char FourWayBL[10] ; 
  char FourWayTR[10] ;
  char FourWayTL[10] ;
  char WideScreenCentral[10] ;
};

//  .ssid[25].password[25].UDP_PORT[5].UDP_ON .Serial_on .ESP_NOW_ON. N2K_ON .Log_ON. log_interval_setting. Data_Log_ON. BLE_enable
struct _sWiFi_settings_Config {  // MAINLY WIFI AND DATA LOGGING key,ssid,PW,udpport, UDP,serial,Espnow
  int EpromKEY;      // Key is changed to allow check for clean EEprom and no data stored change in the default will result in eeprom being reset
                     //  int DisplayPage;   // start pageIndex after defaults
  char ssid[25];
  char password[25];
  char UDP_PORT[5];  // hold udp port as char to make using keyboard easier for now. use atoi when needed!!
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
  bool N2K_ON;
  bool Log_ON;
  int log_interval_setting; //seconds
  bool Data_Log_ON;
  bool BLE_enable;
 };
struct _MyColors {  // for later Day/Night settings
  uint16_t TextColor;
  uint16_t BackColor;
  uint16_t BorderColor;
 // int BoxW;
 // int BoxH;
  int FontH;
  int FontS;
  //bool Simulate;
  int ShowRawDecryptedDataFor;
  bool Frame;
  bool SerialOUT;

};

struct _sInstData {  // struct to hold instrument data AND the time it was updated.  lastx lasty position printed 
  double data = NMEA0183DoubleNA;
  double lastdata = NMEA0183DoubleNA;
  unsigned long updated;
  bool displayed;  // displayed is used by Digital displays
  bool greyed;     // when the data is OLD! 
  bool graphed;    // is used by Graphs, so you can display digital and graph on same pageIndex!
  int source;      // Ready to try an experiment with two GPS to see how they track .
  int lastx,lasty,lasth,lastw;
 
};

struct _sBoatData {
  unsigned long DaysSince1970;  // Days since 1970-01-01

  _sInstData SOG, STW, COG, Latitude, Longitude,MagHeading, TrueHeading, WaterDepth,
    WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM, WindAngleApp, WindAngleGround;
     //_sInstData will be used with NEWUPdate and greys if old

  double SatsInView,Variation, LOCTime, GPSTime, GPSDate,  // keep some GPS stuff in double ..
    Altitude, HDOP, GeoidalSeparation, DGPSAge;
    
  bool MOBActivated;

};
struct GraphBuffer {
  double values[200];
  uint16_t count = 0;
  uint16_t head = 0;

  void push(double val) {
    values[head] = val;
    head = (head + 1) % 200;
    if (count < 200) count++;
  }
  void reset() {
  count = 0;
  head = 0;
  for (uint16_t i = 0; i < 200; i++) {
    values[i] = 0.0;
  }
 } 
  double get(uint16_t i) const {
    return values[(head + i) % 200];
  }
void fill(double val) {
  for (uint16_t i = 0; i < 200; i++) {
    values[i] = val;
  }
  count = 200;
  head = 0;
}



};




//_sButton(h,v,width,height,bordersize, backcol,textcol,bordercol,fontinteger ....); 
struct _sButton { //_sButton(h,v,width,height,bordersize, backC,textC,borderC,Font ; 
  int h, v, width, height, bordersize;
  uint16_t BackColor, TextColor, BorderColor;
  int Font;                  //-1 == not forced (not used?)
  bool Keypressed;           //used by keypressed
  unsigned long LastDetect;  //used by keypressed
  int PrintLine;             // used for UpdateLinef()
    int lastY;                 // used for where last topleft print was  
  bool screenfull,debugpause;
};

struct Phv {   // struct for int positions h v typically on screen 
  int h, v;
};

/// for Victron stuff:


struct _sMyVictronDevices{   // equivalent to _sDisplay_Config all known victron devices MAc and encryption keys.
                //10 index for multiple saved instrument settings first
  bool BLEDebug;
  bool Simulate;
  bool Beacons;
  char charMacAddr[20][13];   // a 12 char (+1!) array  typ= "ea9df3ebc625"  
  char charKey [20][33];      //32 etc...
  char FileCommentName [20][32];  // name from file that will be used in Display
  int displayV[20];
  int displayH[20];
  char DisplayShow[20][10];  // to be used to decide which variables will be displayed = eg V volts I current
  uint8_t VICTRON_BLE_RECORD_TYPE[20];  // INTEGER DESCRIPTOR FOR   DEVICE TYPE (1,2,8 WORK) for use with simulation!
  int displayHeight[20];
  char DeviceVictronName[20][32];  // My DisplayShow to be used to help differentiate devices that give similar VICTRON_BLE_RECORD_TYPE but need more information for a good display
  int ManuDataLength[20]; 
  unsigned char manCharBuf[20][33];  //'Raw' data before formatting as victronManufacturerData  believe 33 is entirely big enough for data so far
  unsigned long updated[20];
  bool displayed[20];  // displayed is used by Digital displays
  bool greyed[20];     // when the data is OLD! 
};

#endif  // _Structures_H_
