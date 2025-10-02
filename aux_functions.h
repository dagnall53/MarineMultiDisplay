#ifndef _AUX_FUN_H_
#define _AUX_FUN_H_
//*** process NMEA packet into Boat variables for display 
#include <Arduino.h>     //necessary for the String, uint16_t etc variable definitions
#include "Structures.h"  // to tell it about the _sBoatData and button structs.
#include <Arduino_GFX_Library.h>  // for the graphics functions
extern Arduino_RGB_Display  *gfx ; //  change if alternate displays !

#define On_Off ? "ON " : "OFF"  // if 1 first case else second (0 or off) same number of chars to try and helps some flashing later
#define True_False ? "true" : "false"

bool processPacket(const char* buf,  _sBoatData &stringBD );
// takes nmea field and places as double in struct data(.data)
// not used outside aux_functions ??
void toNewStruct(char *field, _sInstData &data); 
void toNewStruct(double field, _sInstData &data);   // allow update of struct with simple double data (N2)
double Double_sInstDataAdd(_sInstData &data1, _sInstData &data);//returns a double of value data.data +data1.data;

int HexStringToBytes(const char *hexStr,
                     unsigned char *output,
                     unsigned int *outputLen);

double ValidData(_sInstData variable);  // To avoid showing NMEA0183DoubleNA value in displays etc replace with zero.
  
// other stuff placed here now 
//TBD
void DrawGPSPlot(bool reset, _sButton button,_sBoatData BoatData, double magnification );

#endif