//This file contains definitions for macros

#ifndef _MACROS
#define _MACROS

#include <Arduino.h>

#define MAX_ERRORS 50       //Max number of errors that can be stored in the list of errors
#define MAX_ROOTS 30        //Max number of rows in the T matrix
#define MAX_WIRES_IN_NET 8  //Max number of wires +1 connected to a root wire

#define MASTER_GOOD A14     //Master good LED port  (turns on when all test completed without errors)
#define MASTER_ERROR A15    //Master error LED port (turns on on any error)
#define BUTTON_NEXT A8      //"Next" Button Port
#define BUTTON_RESET A9     //"Reset" Button Port
#define BUTTON_OK A10       //"OK" Button Port

#define POT_LED_SPEED A11   //Port for the led speed controll potentiometer
#define RED_BRIGHTNESS 150   //Controls LED brightness
#define GREEN_BRIGHTNESS 10   //Controls LED brightness
#define CABLE_PRESENT 10    // number to store in the first byte of the EEPROM to mark the presence of a cable template


#endif
