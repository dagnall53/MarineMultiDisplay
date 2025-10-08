/* 
Original version by Dr.András Szép under GNU General Public License (GPL).
but highly modified! 
*/

#include <Arduino.h>  //necessary for the String variables
#include <SPI.h>
#include "aux_functions.h"
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include "debug_port.h"
#include "src/MarinePageGFX.h"  // Double-buffered graphics
#include "CanvasBridge.h"
#include "FontType.h"
#include "Structures.h"
#include "Globals.h"
extern MarinePageGFX* page;
extern GraphBuffer DepthBuffer;


extern int MasterFont;
//extern void setFont(int);

extern int Display_Page;
extern _MyColors ColorSettings;
extern _sDisplay_Config Display_Config;
extern void showPictureFrame(_sButton &button, const char *name);
extern int Screen_Width;

extern double NMEA0183GetDouble(const char *data);  // have to do this as its local to NMEA0183Messagesmessages.cpp!

extern char *pTOKEN;  // const ?global? pointer of type char, used to get the fields in a nmea sentence
                      // when a new sentence is processed we advance this pointer n positions
                      // after the beginning of the sentence such that it points to the 1st field.
                      // used by function void FillToken(char * ptr) in

char Field[20][18];  // for the extracted NMEA fields

#define MAX_NMEA_FIELDS 64
char nmeaLine[MAX_NMEA0183_MSG_BUF_LEN];  //NMEA0183 message buffer
size_t i = 0, j = 1;                      //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)
int noOfFields = MAX_NMEA_FIELDS;         //max number of NMEA0183 fields
int Num_Conversions, Num_DataFields;      //May be needed in sub functions?

int TokenCount(char *ptr) {
  // to count number of  commas in the char array (counts number of data fields present)
  char ch = ',';
  int count = 0;
  for (int i = 0; i < strlen(ptr); i++) {
    if (ptr[i] == ch) count++;
  }
  return count;
}

boolean FillToken(char *ptr) {  //
  /* the function receives a pointer to the char array where the field is to be stored.
   * It uses the global pointer pTOKEN that points to the starting of the field that we
   * want to extract. So we search for the next field delimiter "," which will be pointed
   * by p2 and we get the lenght of the field in the variable len. We copy len characters
   * to the destination char array and terminate the array with a 0 (ZERO). So if a field
   * is empty, len = 0 and ptr[0]=0. This can be used, later, to test if the field was empty.
   * Finally as p2 is pointing to a "," we increment it by 1 and copy it to pTOKEN so that
   * pTOKEN will be pointing to the starting of the next field.
   */
  char *p2 = strchr(pTOKEN, ',');
  if (p2 == NULL) { return false; }
  int len = p2 - pTOKEN;
  memcpy(ptr, pTOKEN, len);
  ptr[len] = 0;
  pTOKEN = p2 + 1;
  return true;
}

boolean FillTokenLast(char *ptr) {
  // All NMEA messages should have the checksum, with a * delimeter for the last data field. -- but some coders may forget this and just end with CR..(!)
  // This is the same basic code as the FillToken code Luis wrote, but searches for '*' and if not found, looks for a CR.
  // It is ONLY used to extract the last expected datafield from NMEA messages.  - And we counted the number of data fields using a ", count" in  TokenCount.
  // Therefore this will extract the last datafield from NMEA data messages with either  a * or a CR  delimiting the last datafield.
  // could probably be written in a more elegant way..
  char *p2 = strchr(pTOKEN, '*');
  char *p3 = strchr(pTOKEN, 0x0D);
  if ((p2 == NULL) && (p3 == NULL)) { return false; }
  if (p2 == NULL) {
    int len = p3 - pTOKEN;  // Second choice, "*" was not found/missing so just extract ALL the remaining data before the CR.
    memcpy(ptr, pTOKEN, len);
    ptr[len] = 0;
    pTOKEN = p3 + 1;           // not a lot of point in this as this was the end of the message.. ?
  } else {                     // not yet at end of message! We found a "*"
    int len = p2 - pTOKEN;     // get length from last comma to "*" character
    memcpy(ptr, pTOKEN, len);  // copy whatever there is to ptr
    ptr[len] = 0;              // place the end mark (0)
    pTOKEN = p2 + 1;           // could probably remove this as this subroutine is only called for the last data field. ?
  }
  return true;
}

extern void EventTiming(String input);  // to permit timing functions here during development
// looks for index where three characters match my "haystack" of known NMEA messages;
bool NeedleinHaystack(char ch1, char ch2, char ch3, char *haystack, int &compareOffset) {
  // DEBUG_PORT.printf("\n Looking for<%c%c%c> in strlen(%i) %s \n", ch1, ch2, ch3, strlen(haystack), haystack);
  compareOffset = 0;
  // if (needle[0] == '\0') { return false; }
  for (compareOffset = 0; (compareOffset <= strlen(haystack)); compareOffset++) {
    if ((ch1 == haystack[compareOffset]) && (ch2 == haystack[compareOffset + 1]) && (ch3 == haystack[compareOffset + 2])) {
      //    DEBUG_PORT.printf("Found at %i\n", compareOffset);
      return true;
    }
  }
  compareOffset = 0;
  return false;
}

//********* Add this if needed in the case statements to help sort bugs!
// DEBUG_PORT.println(" Fields:");
// for (int x = 0; x <= Num_DataFields; x++) {
//   DEBUG_PORT.print(Field[x]);
//   DEBUG_PORT.print(",");
// }
// DEBUG_PORT.println("> ");
/* ref  from TL functions..
double NMEA0183GetDouble(const char *data) {
  double val = NMEA0183DoubleNA;
  if (data == 0) return val;         // null data sets a (detectable but should have no effect) 1e-9 
  for (; *data == ' '; data++);      // Pass spaces
  if (*data != 0 && *data != ',') {  // not empty field
    val = atof(data);
  }
  return val;
}
*/

double ValidData(_sInstData variable) {  // To avoid showing NMEA0183DoubleNA value in displays etc replace with zero.
  double res = 0;
  if (variable.greyed) { return 0; }
  if (variable.data != NMEA0183DoubleNA) { res = variable.data; }
  return res;
}
//revised 18/03 all NULL data should be set "grey"
void toNewStruct(char *field, _sInstData &data) {
  data.greyed = true;
  data.updated = millis();
  data.lastdata = data.data;
  data.data = NMEA0183GetDouble(field);  // if we have included the TL library we do not need the function copy above
  if (data.data != NMEA0183DoubleNA) {
    data.greyed = false;
  }
  data.displayed = false;
  data.graphed = false;
}
void toNewStruct(double field, _sInstData &data) {  // allow update of struct with simple double data
  data.greyed = true;
  data.updated = millis();
  data.lastdata = data.data;
  data.data = field;
  if (data.data != NMEA0183DoubleNA) {
    data.greyed = false;
  }
  data.displayed = false;
  data.graphed = false;
}
// reads char array buf and places (updates) data if found in stringND
bool processPacket(const char *buf, _sBoatData &BoatData) {  
  char *p;
  int Index;
  int Num_Conversions = 0;
  Num_DataFields = TokenCount(pTOKEN);  //  TokenCount reads the number of commas in the pointed nmea_X[] buffer
  pTOKEN = pTOKEN + 1;                  // pTOKEN points advances $/!
  for (int n = 0; n <= Num_DataFields - 1; n++) {
    p = Field[n];
    if (!FillToken(Field[n])) { return false; }  // FillToken looks for "," places the data pointed to into Field[n]
  }
  p = Field[Num_DataFields];  // searches for '*' and if not found, looks for a CR
  if (!FillTokenLast(Field[Num_DataFields])) { return false; }
  //DEBUG_PORT.printf("  Found  <%i> Fields Field0<%s> Field1<%s> Field2<%s> Field3<%s>\n", Num_DataFields, Field[0],Field[1], Field[2], Field[3]);
  //NeedleInHaystack/4/will (should !) identify the command.  Note Nul to prevent zero ! being passed to Switch or Div4
  //                  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19....
  char nmeafunct[] = "NUL,DBT,DPT,DBK,MWV,VHW,RMC,APB,GLL,HDG,HDM,MTW,MWD,NOP,XS,,AK,,ALK,BWC,WPL,GSV ";  // more can be added..
  // Not using Field[0] as some commands have only two characters. so we can look for (eg) 'XS,' from $IIXS, =14
  if (NeedleinHaystack(buf[3], buf[4], buf[5], nmeafunct, Index) == false) { return false; }
  // NOTE  I coul have missed out the commas in the nmeafunct[]] "haystack", and divided by 3.. but that would make it harder to read. 
  //DEBUG_PORT.printf(" Using case %i \n", Index / 4);
  // DEBUG_PORT.println(" Fields:");for(int x=0 ;int <Num_DataFields;int++){DEBUG_PORT.print(Field[x]);DEBUG_PORT.print(",");} DEBUG_PORT.println("> ");
  switch (Index / 4) {
    case 1:  //dbt
      toNewStruct(Field[3], BoatData.WaterDepth);DepthBuffer.push(BoatData.WaterDepth.data);// depth is negative! to keep the graph sensible
      return true;
      break;
    case 2:  //DPT //dIFFERENT TO DBT/DBK
      toNewStruct(Field[1], BoatData.WaterDepth);DepthBuffer.push(BoatData.WaterDepth.data);
      return true;
      break;
    case 3:  //DBK
      toNewStruct(Field[3], BoatData.WaterDepth);DepthBuffer.push(BoatData.WaterDepth.data);
      return true;
      break;

    case 4:  //mwv
      toNewStruct(Field[1], BoatData.WindAngleApp);
      toNewStruct(Field[3], BoatData.WindSpeedK);
      // also try to compute Ground wind.. Relative to North
      toNewStruct((BoatData.Variation + Double_sInstDataAdd(BoatData.WindAngleApp, BoatData.MagHeading)), BoatData.WindAngleGround);


      return true;
      break;

    case 5:  //VHW
      toNewStruct(Field[5], BoatData.STW);
      // other VHW data (directions!) are usually false!
      return true;
      break;
    case 6:  //RMC
      toNewStruct(Field[7], BoatData.SOG);
      toNewStruct(Field[8], BoatData.COG);
      // nmea0183nan (-10million.. so may need extra stuff to prevent silly displays!)

      BoatData.Latitude.data = LatLonToDouble(Field[3], Field[4][0]);   // using TL's functions that return null value
      BoatData.Longitude.data = LatLonToDouble(Field[5], Field[6][0]);  //nb we use +1 on his numbering that omits the command

      BoatData.GPSTime = NMEA0183GPTimeToSeconds(Field[1]);
      BoatData.LOCTime = NMEA0183GPTimeToSeconds(Field[1]) + 3600 * Display_Config.LocalTimeOffset;

      while ((int(BoatData.LOCTime) / 3600) >= 24) { BoatData.LOCTime = BoatData.LOCTime - 86400; }
      while (int(int(BoatData.LOCTime) / 3600) < 1) { BoatData.LOCTime = BoatData.LOCTime + 86400; }
      while ((int(BoatData.LOCTime) / 3600) >= 24) { BoatData.LOCTime = BoatData.LOCTime - 86400; }  //catch wrap around
      BoatData.GPSDate = atof(Field[9]);
      // mag variation is [10]/[11] (E) But I think this comes from a look up and not the satellites at least on my cheapo GPS module.

      return true;  //
      break;
    case 7:  //APA  3=xte 8 = bearing to dest (9=M agnetic or  T rue)
             //APB  3= xte 11 = CURRENT BEARING TO DEST  and 12(m/t) same..AS APA

      return true;
      break;
    case 9:  //HDG
      toNewStruct(Field[1], BoatData.MagHeading);
      return true;
      break;



    case 10:  //HDM
      toNewStruct(Field[1], BoatData.MagHeading);
      return true;
      break;

    case 19:  //GSV
      // DEBUG_PORT.printf("\n Debug GSV ? numdatafields<%i>  ", Num_DataFields);
      //          if (Num_DataFields < 10) { return false; }
      //        DEBUG_PORT.println("Fields<");                  // un copy this lot to assist debugging!!
      //        for (int x = 0; x <= Num_DataFields; x++) {
      //          DEBUG_PORT.printf("%i=<%s>,",x,Field[x]);
      //        }
      //        DEBUG_PORT.println(" ");
      //  Not new struct, sats in view is just a double. not an inststruct. toNewStruct(Field[3], BoatData.SatsInView);
      BoatData.SatsInView = NMEA0183GetDouble(Field[3]);

      return true;
      break;

    default:
      return false;
      break;
  }


  return false;
}

void DrawGPSPlot(bool reset, _sButton &BLOB, _sBoatData BoatData, double magnification) { //do later 
  static double startposlat, startposlon;
  double LatD, LongD;  //deltas
  int h, v;
  if (BoatData.Latitude.data != NMEA0183DoubleNA) {
    if (reset) {
      startposlat = BoatData.Latitude.data;
      startposlon = BoatData.Longitude.data;
    }
     h = 0 + ((Screen_Width) / 2);  // screen width / height 
     v = 0  + ((480) / 2);
                 BLOB.h=0;
            BLOB.v=0;
            BLOB.BackColor=WHITE;
            page->DrawBox(BLOB); 
    // magnification 1 degree is roughly 111111 m
    if (startposlon == 0) {
      startposlat = BoatData.Latitude.data;
      startposlon = BoatData.Longitude.data;
    }
    LongD = h + ((BoatData.Longitude.data - startposlon) * magnification);
    LatD = v - ((BoatData.Latitude.data - startposlat) * magnification);  // negative because display is top left to bottom right!
            BLOB.h=LongD;
            BLOB.v=LatD;
            BLOB.BackColor=BLUE;
            BLOB.BorderColor=WHITE;
            page->DrawBox(BLOB);                                                                     //set limits!! ?
   }
}







double Double_sInstDataAdd(_sInstData &data1, _sInstData &data) {
  double temp;
  temp = 0;
  if (data.data != NMEA0183DoubleNA) { temp += data.data; }
  if (data1.data != NMEA0183DoubleNA) { temp += data1.data; }
  return temp;
}
//https://gist.github.com/xsleonard/7341172?permalink_comment_id=2372748
int HexStringToBytes(const char *hexStr,
                     unsigned char *output,
                     unsigned int *outputLen) {
  size_t len = strlen(hexStr);
  if (len % 2 != 0) {
    return -1;
  }
  size_t finalLen = len / 2;
  *outputLen = finalLen;
  for (size_t inIdx = 0, outIdx = 0; outIdx < finalLen; inIdx += 2, outIdx++) {
    if ((hexStr[inIdx] - 48) <= 9 && (hexStr[inIdx + 1] - 48) <= 9) {
      goto convert;
    } else {
      if (((hexStr[inIdx] - 65) <= 5 && (hexStr[inIdx + 1] - 65) <= 5) || ((hexStr[inIdx] - 97) <= 5 && (hexStr[inIdx + 1] - 97) <= 5)) {
        goto convert;
      } else {
        *outputLen = 0;
        return -1;
      }
    }
    convert:
    output[outIdx] =
      (hexStr[inIdx] % 32 + 9) % 25 * 16 + (hexStr[inIdx + 1] % 32 + 9) % 25;
  }
  output[finalLen] = '\0';
  return 0;
}
