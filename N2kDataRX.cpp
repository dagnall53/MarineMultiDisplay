/*
N2KdataRX.cpp

Copyright (c) 2015-2018 Timo Lappalainen, Kave Oy, www.kave.fi
Adding AIS (c) 2019 Ronnie Zeiller, www.zeiller.eu

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


my work here is based on example in Examples  N2KdataRX.cpp
*/

#include "N2KdataRX.h"
#include <N2kMessages.h>
#include <N2kTypes.h>
#include <NMEA0183Messages.h>
#include <math.h>

#include <math.h>
#include <string.h>

const double radToDeg = 180.0 / M_PI;
#include "aux_functions.h"
#include "Structures.h"

extern _sBoatData BoatData;  // BoatData values for the display , int double , when read, when displayed etc 




//*****************************************************************************
void tN2KdataRX::HandleMsg(const tN2kMsg &N2kMsg) {

  switch (N2kMsg.PGN) {
    // these update my structures on receipt:
    case 127250UL: HandleHeading(N2kMsg); break;  //
    case 128259UL: HandleBoatSpeed(N2kMsg); break;
    case 128267UL: HandleDepth(N2kMsg); break;
    case 129026UL: HandleCOGSOG(N2kMsg); break;
    case 130306UL: HandleWind(N2kMsg); break;
    // under test...
    case 127245UL: HandleRudder(N2kMsg); break;  // 127245

    case 130312UL: HandleWatertemp12(N2kMsg); break;
    case 130316UL: HandleWatertemp16(N2kMsg); break;

      //note we may later decide to add 130312 and 130314 PGNs that replace 130311

    case 127258UL: HandleVariation(N2kMsg); break;
    case 129025UL: HandlePosition(N2kMsg); break;  // lat long
    case 129029UL: HandleGNSS(N2kMsg); break;
    case 126992UL: HandleGNSSSystemTime(N2kMsg); break;

    default: return;
  }
}




//*****************************************************************************

//****************************************************
void tN2KdataRX::Update() {  // note other messages will be initiated immediately by their tN2KdataRX HandleMsg
  static bool ResetDone = false;
  // On the First Run, RESET the variables that are used as "indicators"  so that we do not get spurious Data sent on startup.
  if (!ResetDone) {
    ResetDone = true;  
    Heading = N2kDoubleNA;
    COG = N2kDoubleNA;
    SOG = N2kDoubleNA;
    WindSpeed = N2kDoubleNA;
    WindAngle = N2kDoubleNA;
  }

}

//*****************************************************************************
void tN2KdataRX::HandleHeading(const tN2kMsg &N2kMsg) {
  /*
  1 Sequence ID
  2 Heading Sensor Reading
  3 Deviation
  4 Variation
  5 Heading Sensor Reference
  6 NMEA Reserved
  {"Vessel Heading", https://github.com/canboat/canboat/blob/master/analyzer/pgn.h
     127250,
     PACKET_COMPLETE,
     PACKET_SINGLE,
     {UINT8_FIELD("SID"),
      ANGLE_U16_FIELD("Heading", NULL),
      ANGLE_I16_FIELD("Deviation", NULL),
      ANGLE_I16_FIELD("Variation", NULL),
      LOOKUP_FIELD("Reference", 2, DIRECTION_REFERENCE),
      RESERVED_FIELD(6),
      END_OF_FIELDS},
     .interval = 100}
     https://canboat.github.io/canboat/canboat.html#pgn-list So gets..  0SID,1Heading,2Deviation,3Variation,4 Dirref,5Reserved, 
     ParseN2kPGN127250  sets index=0 and gets SID,Heading,Deviation,Variation,refref=(tN2kHeadingReference)
  */
  unsigned char SID;
  tN2kHeadingReference ref;
  double Deviation = NMEA0183DoubleNA;  // not used in other places ?
  double _Deviation = 0;
  double _Variation;

  bool SendHDM = true;
  if (ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, _Variation, ref)) {
    if (ref == N2khr_magnetic) {
      if (!N2kIsNA(_Deviation)) {
        Deviation = _Deviation;
        SendHDM = false;
      }  // Update Deviation, send HDG
      if (!N2kIsNA(_Variation)) {
        Variation = _Variation;
        SendHDM = false;
      }  // Update Variation, Send HDG
      if (!N2kIsNA(Heading) && !N2kIsNA(_Deviation)) { Heading -= Deviation; }
      if (!N2kIsNA(Heading) && !N2kIsNA(_Variation)) { Heading -= Variation; }
      toNewStruct(Heading, BoatData.MagHeading);
       } else {  // data was "true" so send as true
      toNewStruct(Heading, BoatData.TrueHeading);
    }
  }
}



//***************************************

// //*********************  N2000 PILOT FUNCTIONS ***************************
// // *********  Define a Structure for waypoints so we can index them in PGN 129285 - we only use 2 - but it is possible we may get more.
// //
// struct tWAYPOINT {
//   uint16_t ID;
//   char Name[20];
//   double latitude;
//   double longitude;
// };

// /*
// double DistanceToWaypoint,ETATime,BearingOriginToDestinationWaypoint,BearingPositionToDestinationWaypoint, DestinationLatitude,DestinationLongitude,WaypointClosingVelocity;
//   int16_t ETADate;
//   uint32_t OriginWaypoint_index,DestinationWaypoint_index; 
//   bool PerpendicularCrossed,ArrivalCircleEntered;
//   tN2kHeadingReference BearingReference;
//   tN2kDistanceCalculationType CalculationType;
// */


// struct tROUTE {
//   uint16_t StartRPS;
//   uint16_t nItems;
//   int16_t DatabaseID;
//   uint16_t RouteID;
//   uint8_t suppData;
//   char Name[20];
//   uint32_t OriginWaypoint_index;
//   uint32_t DestinationWaypoint_index;
//   tN2kHeadingReference BearingReference;
//   tN2kDistanceCalculationType CalculationType;
// };

// tWAYPOINT Waypoint[5];
// tROUTE MyRoute;


//*****************************************************************************
void tN2KdataRX::HandleVariation(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kMagneticVariation Source;
  uint16_t LOCALDaysSince1970;
  // Just saves the Variation for use in other functions.
  ParseN2kMagneticVariation(N2kMsg, SID, Source, LOCALDaysSince1970, Variation);
  BoatData.Variation = Variation; // just save value, not sInstData so not need to save time of data etc.. 
  }


//*****************************************************************************
void tN2KdataRX::HandleBoatSpeed(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double WaterReferenced;
  double GroundReferenced;
  tN2kSpeedWaterReferenceType SWRT; // water speed reference type 
  // ignore ground referenced! 
  if (ParseN2kBoatSpeed(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT)) {
     toNewStruct(WaterReferenced, BoatData.STW);
    
  }
}

//*****************************************************************************
void tN2KdataRX::HandleDepth(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range)) {
    toNewStruct(DepthBelowTransducer, BoatData.WaterDepth);
     }
}

//*****************************************************************************
void tN2KdataRX::HandlePosition(const tN2kMsg &N2kMsg) {

  if (ParseN2kPGN129025(N2kMsg, Latitude, Longitude)) {
// needs toNewStruct(DepthBelowTransducer, BoatData.WaterDepth);
  }
  
}

//*****************************************************************************
void tN2KdataRX::HandleCOGSOG(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference HeadingReference;


  if (ParseN2kCOGSOGRapid(N2kMsg, SID, HeadingReference, COG, SOG)) {
    //get / set  MCOG
    MCOG = (!N2kIsNA(COG) && !N2kIsNA(Variation) ? COG - Variation : NMEA0183DoubleNA);
    if (HeadingReference == N2khr_magnetic) {
      MCOG = COG;
      if (!N2kIsNA(Variation)) COG -= Variation;
    }
    toNewStruct(COG, BoatData.COG);
    toNewStruct(SOG, BoatData.SOG);
    
  }
}

//*****************************************************************************
void tN2KdataRX::HandleGNSS(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kGNSStype GNSStype;
  tN2kGNSSmethod GNSSmethod;
  unsigned char nSatellites;
  double HDOP;
  double PDOP;
  double GeoidalSeparation;
  unsigned char nReferenceStations;
  tN2kGNSStype ReferenceStationType;
  uint16_t ReferenceSationID;
  double AgeOfCorrection;
  /*
 
*/
  if (ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                   nSatellites, HDOP, PDOP, GeoidalSeparation,
                   nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection)) {
    // do not have Position structure? toNewStruct(COG, BoatData.COG);

  }
}

void tN2KdataRX::HandleGNSSSystemTime(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  uint16_t SystemDate;
  double SystemTime;
  tN2kTimeSource TimeSource;

  if (ParseN2kSystemTime(N2kMsg, SID, SystemDate, SystemTime, TimeSource)) {
    time_t t = tNMEA0183Msg::daysToTime_t(SystemDate);
    tmElements_t tm;
    tNMEA0183Msg::breakTime(t, tm);
    int GPSDay = tNMEA0183Msg::GetDay(tm);
    int GPSMonth = tNMEA0183Msg::GetMonth(tm);
    int GPSYear = tNMEA0183Msg::GetYear(tm);
    int LZD = 0;
    int LZMD = 0;
    //toNewStruct(TIME, BoatData.TIME);
    // needs setStruct
  }
}

void tN2KdataRX::HandleWatertemp12(const tN2kMsg &N2kMsg) {
  // Garmin depth sensor output as advised Erasmo J. D. Chiappetta Filho 02/06/25 
  unsigned char SID, TempInstance;
  tN2kTempSource TempSource;
  double SeaTemp, SetTemperature;
  SeaTemp = NMEA0183DoubleNA;
  if (ParseN2kPGN130312(N2kMsg, SID, TempInstance, TempSource, SeaTemp, SetTemperature)) {
    if (TempSource != N2kts_SeaTemperature) { return; }
  //  toNewStruct(SeaTemp, BoatData.SeaTemp);
    
  }
}
void tN2KdataRX::HandleWatertemp16(const tN2kMsg &N2kMsg) {
  // Garmin depth sensor output  advised  Erasmo J. D. Chiappetta Filho 02/06/25 
  unsigned char SID, TempInstance;
  tN2kTempSource TempSource;
  double SeaTemp, SetTemperature;
  SeaTemp = NMEA0183DoubleNA;
  if (ParseN2kPGN130316(N2kMsg, SID, TempInstance, TempSource, SeaTemp, SetTemperature)) {
    if (TempSource != N2kts_SeaTemperature) { return; }
    //  toNewStruct(SeaTemp, BoatData.SeaTemp);
    
  }
}

//*****************************************************************************
void tN2KdataRX::HandleWind(const tN2kMsg &N2kMsg) {
  /* see C:\Users\admin\Documents\Arduino\libraries\NMEA2000\Examples\NMEA2000ToNMEA0183\N2KdataRX.cpp
*/


  unsigned char SID;
  tN2kWindReference WindReference;                               // NOTE this is local and N2K and different from the tNMEA0183WindReference
                                                                 //               that we store as (int)   00 ground 01ground north ref 02apparent waterlin 03 theoretical using cog sog 04 centerline theoretical based on water speed
                                                                 // 00=T & 01=T 02,03,04=A
  tNMEA0183WindReference NMEA0183Reference = NMEA0183Wind_True;  // also LOCAL! just true and apparent

  if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference)) {
    WindSourceApparent = false;  // this is the variable we will save for elsewhere
    if (WindReference == N2kWind_Apparent) {
      WindSourceApparent = true;
      NMEA0183Reference = NMEA0183Wind_Apparent;
    }
    // DO we need to check for N2KWind_
    //  uncomment to send immediately
    if (WindSourceApparent) {
      NMEA0183Reference = NMEA0183Wind_Apparent;
    } else {
      NMEA0183Reference = NMEA0183Wind_True;
    };
    toNewStruct(WindAngle * radToDeg, BoatData.WindAngleApp);
    toNewStruct(WindSpeed, BoatData.WindSpeedK);
    //
  }
}

//*****************************************************************************
void tN2KdataRX::HandleRudder(const tN2kMsg &N2kMsg) {

  unsigned char Instance;
  tN2kRudderDirectionOrder RudderDirectionOrder;
  double AngleOrder;

  if (ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder)) {
  //set  a rudder position structure? 

  }
}






