// This file contains the definitions of core structures for this application

#ifndef _CORE_STRUCTS
#define _CORE_STRUCTS

#include "Macros.h"

//STRUCTURES
struct modes  //Contains all UI related to operation modes
{
    int mode;    //1 fast, 2 stop on error, 3 blind
    char description[30]; //short description of the mode
    int led_speed = 10;   //the delay time between consective checks
};

struct cable
{
  /*
  # This is a description of the format used to save a wire template for a cable.

  ## Definitions
  Cable = a collection of wires with a specific arrangement of the wires within.
  Wire = a connection between an input pin and one or more output pins.
  Input pin = a pin that is an output of the arduino board and is considered an input to a "pin" in the cable connector.
  Output pin = a pin that is an input of the arduino board and is considered the output of a "pin" in the cable connector.
  Pin ID = the unique number given to a pin from the cable. The ID-s start from 0. ID-s must be consecutive. The input pins have the lowest ID-s.
  Wire template = the correct arrangement of wires within a cable.

  This is a cable

      ######################           ######################
    ---|--------------------|-----------|--------------------|---
    ---|--------------------|-----------|--------------------|---
    ---|--------------------|-----------|--------------------|---
      ######################           ######################

  Input pins     Input connector     Wires       Output connector     Output pins


  ## The template is saved within a 2-dimensional array.
  We use a 2-dimensional array for input pins that connect to multiple output pins.

  T - the array whitch holds the template (The "golden sample" specifing the connections between the input pins and the output pins = the wire configuration in the cable)

  T[0] - 1-dimensional array with the id-s of the pins connected to the first input pin
  T[1] - 1-dimensional array with the id-s of the pins connected to the second input pin
  ... 
  T[n] - 1-dimensional array with the id-s of the pins connected to the first n'th pin

  T[1][1] = k - k is the ID of the output pin connected to input pin 1.
  T[1][2] = m - m is the ID of the output pin connected to input pin 1. // For input pins with multiple output pins

  After the last pin int in the row, a special value is added to mark the end of the row. This special value is -1.

  ## EXAMPLE

        ######################           ######################
    0---|--------------------|-----------|--------------------|---3
    1---|--------------------|-----------|--------------------|---4
    2---|--------------------|-----------|--------------------|---5
        ######################           ######################

  The array is:
  T[0] : 3 -1
  T[1] : 4 -1
  T[2] : 5 -1

  ## EXAMPLE 2
                                                    ######################
        ######################     |----------|--------------------|---3
    0---|--------------------|-----|----------|--------------------|---4
    1---|--------------------|----------------|--------------------|---5
    2---|--------------------|----------------|--------------------|---6
        ######################                ######################

  The array is:
  T[0] : 3 4 -1
  T[1] : 5 -1
  T[2] : 6 -1
  */

  int tot_pins;     //Total number of pins on the cable
  int CI = -1;      //Last sequential pin number of INPUT CONNECTOR
  int CO_1 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 1
  int CO_2 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 2
  int CO_3 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 3

  int T[MAX_ROOTS][MAX_WIRES_IN_NET];
};


struct err
{
  int err_count = 0;
  int err_list[MAX_ERRORS][3];  //Containes [][0] ID of originating pin
                                //          [][1] error type  (0 - lack of continuity | 1 - mismatch (wrong destination pin) | 3 - short circuit DEPRECATED (destination pin is an input pin))
                                //          [][2] ID of correct destination pin
};


struct hardware_model
{
  //Containes a description of present hardware options
  bool lcd; //true if LCD present
  int board;//:0 - arduino mega, 1 - arduino uno, ...
};


#endif
