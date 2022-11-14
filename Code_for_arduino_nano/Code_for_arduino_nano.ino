#define MAX_ERRORS 10       //Max number of errors that can be stored in the list of errors
#define MASTER_GOOD A0      //Master good LED port  (turns on when all test completed without errors)
#define MASTER_ERROR A1     //Master error LED port (turns on on any error)
#define BUTTON_NEXT A2      //"Next" Button Port
#define BUTTON_RESET A3     //"Reset" Button Port
#define BUTTON_OK 12        //"OK" Button Port

#define RED_BRIGHTNESS 70   //Controls LED brightness

#include <SoftPWM.h>        //by Brett Hagman
//https://github.com/bhagman/SoftPWM  V1.0.1
#include <LiquidCrystal_I2C.h>
//https://github.com/johnrickman/LiquidCrystal_I2C  V1.1.2

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

  int T[50][5];
};

struct err
{
  int err_count = 0;
  int err_list[MAX_ERRORS][3];  //Containes [][0] ID of originating pin
                                //          [][1] error type  (0 - lack of continuity | 1 - mismatch (wrong destination pin) | 3 - short circuit (destination pin is an input pin))
                                //          [][2] ID of correct destination pin
};


//Declare global variables
int reset_happened = 0;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file
cable C;
//Declare a modes variable for user interface info
modes State;

//Read from csv!
// available pins in this array
//int HW_P[] = {2, 3, 4, 5, 6, 7, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, A0, A1, A2, A3, A4, A5, A6, A7};
////            0  1  2  3  4  5  6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  41  42  43  44

//For arduino nano
int HW_P[] = {2, 3, 5, 6, 7, 8, 9, 10, 11};
//            0  1  2  3  4  5  6  7   8   9   10  11
//See how to read from csv file


void concat_errors(err *master, err *addition)
{
    //Concatenates errors from 1 pin to the list of errors of the cable
    //master - the list of errors of the cable
    //addition - the list of errors of the current (input)pin/wire
    int r = master->err_count;
    master->err_count += addition->err_count;

    int b = 0;
    for(int a = r; a < master->err_count; a++)
    {
        master->err_list[a][0] = addition->err_list[b][0];
        master->err_list[a][1] = addition->err_list[b][1];
        master->err_list[a][2] = addition->err_list[b][2];
        b++;
    }
}

err check_pin(cable C, const int i, const modes OP_mode)
{
  //Checks the (input)pin from the cable
  //C - The wiring configutration of the cable under test
  //i - The (input)pin that needs to be checked
  //OP_mode - The operation mode
  //err - The returned list of errors
  
  //TO DO
  //Check if i belongs to the input connector
  err report;
  report.err_count = 0;

  //Make sure MASTER_GOOD LED is NOT on when testing
  SoftPWMSet(MASTER_GOOD, 0);

  pinMode(HW_P[i], OUTPUT);
  digitalWrite(HW_P[i], HIGH);
  delay(10);  //Wait to settle

  int j = 0;
  while(C.T[i][j] != -1)  //Check for (correct) conectivity
  {
    if(digitalRead(HW_P[C.T[i][j]]) == LOW)
    {
      if(report.err_count < MAX_ERRORS)
      {
        SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 0;          //error ID (in this case open circuit)
        report.err_list[report.err_count][2] = C.T[i][j];  //destination pin
        report.err_count ++;

        if(OP_mode.mode == 2)
        {
          //wait for input from the operator
          delay(500); //Do not register previous long presses
          while(1)
          {
            if(reset_happened == 0)
            {
              reset_happened = !digitalRead(BUTTON_RESET);
            }
            char next = Serial.read();
            if(digitalRead(BUTTON_NEXT) == 0 || next == 'c' || next == 'C' || reset_happened)
            {
              while(Serial.read() >= 0)
              {
                ; //Empty Serial buffer
              }
              break;  //exit infinite loop
            }
          }
        }

      }
    }
    j++;
  }
  
  for(j = 0; j < C.tot_pins; j++)   //Check for mismatch or shorts
  {
    //Only check wires that have not been allready tested
    int not_tested = 1;
    int k = 0;
    while(C.T[i][k] != -1)  //go trough the current template line
    {
      if (j == C.T[i][k])
      {
        not_tested = 0;
      }
      k++;
    }
    if(j == i)
    {
      not_tested = 0;
    }

    if(not_tested == 1)   //Checking wires that are not_tested
    {
      //check for short-circuit errors
      if(j <= C.CI && digitalRead(HW_P[j]) == HIGH && i < j)
      {
        //digitalWrite(MASTER_ERROR, HIGH);
        SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 3;          //error ID (in this case short circuit)
        report.err_list[report.err_count][2] = j;  //destination pin
        report.err_count ++;
      }
      //check for mismatch errors
      if(j > C.CI && j <= C.CO_3 && digitalRead(HW_P[j]) == HIGH)
      {
        //digitalWrite(MASTER_ERROR, HIGH);
        SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 1;          //error ID (in this case mismatch)
        report.err_list[report.err_count][2] = j;  //destination pin
        report.err_count ++;
      }
    }
  }
  delay(OP_mode.led_speed);

  //Prints errors to arduino serial interface
  print_S(report);

  //Stop on error mode
  if(OP_mode.mode == 2 && report.err_count > 0)
  {
    delay(500); //Do not register previous long presses
    while(1)
    {
      if(reset_happened == 0)
      {
        reset_happened = !digitalRead(BUTTON_RESET);
      }
      char next = Serial.read();
      if(digitalRead(BUTTON_NEXT) == 0 || next == 'c' || next == 'C' || reset_happened)
      {
        while(Serial.read() >= 0)
        {
          ; //Empty Serial buffer
        }
        break;  //exit infinite loop
      }
    }
  }

  //Put current pin back to INPUT and Return results
  digitalWrite(HW_P[i], LOW);
  pinMode(HW_P[i], INPUT);
  delay(1);
  return report;
}

void print_err_str_lcd(err *errors)
{
    //writes the list of errors on the LCD
    for(int current_error = 0; current_error < errors->err_count; current_error++)
    {
        //Set a HIGH signal on current faulty pin
        pinMode(HW_P[errors->err_list[current_error][0]], OUTPUT);
        digitalWrite(HW_P[errors->err_list[current_error][0]], HIGH);
        
        //Print current pin ID
        lcd.setCursor(0, 0);
        lcd.print(F("IN_P:"));
        if(errors->err_list[current_error][0] < 9)
        {
            lcd.print(errors->err_list[current_error][0]);
            Serial.print("^^^");
            Serial.print(errors->err_list[current_error][0]);
            Serial.print("^^^");
            lcd.print(F(" "));
        }
        else
        {
            lcd.print(errors->err_list[current_error][0]);
        }

        //Space
        lcd.print(F(" "));

        //Print current error number
        if(current_error < 8)
        {
            lcd.print(current_error + 1);
            lcd.print(F(" "));
        }
        else
        {
            lcd.print(current_error + 1);
        }
        lcd.print(F("/"));
        if(errors->err_count < 9)
        {
            lcd.print(errors->err_count);
            lcd.print(F(" "));
        }
        else
        {
            lcd.print(errors->err_count);
        }

        //Next row
        lcd.setCursor(0, 1);
        lcd.print(errors->err_list[current_error][0]);
        lcd.print(F(" "));
        lcd.print(errors->err_list[current_error][1]);
        lcd.print(F(" "));
        lcd.print(errors->err_list[current_error][2]);
        lcd.print(F(" "));

        //DEBUG
        delay(400);
        while(digitalRead(BUTTON_NEXT) == 1);

        //Set current faulty pin to INPUT LOW
        digitalWrite(HW_P[errors->err_list[current_error][0]], LOW);
        pinMode(HW_P[errors->err_list[current_error][0]], INPUT);
    }
}

void print_S(err report)
{
  //Prints the error report on the arduino serial interface
  if(report.err_count != 0)
  {
    int i = 0;
    Serial.print(F("Number of errors on in_pin "));
    Serial.print(report.err_list[i][0]);
    Serial.print(F(": "));
    Serial.println(report.err_count);
  
    while(i < report.err_count && i < MAX_ERRORS)
    {
      Serial.print(report.err_list[i][0]);
      Serial.print(F(" "));
      Serial.print(report.err_list[i][1]);
      Serial.print(F(" "));
      Serial.println(report.err_list[i][2]);
      i++;
    }
  }
}



void setup()
{
  //Initializing the cable
  /*
  C.tot_pins = 4;
  C.CI = 1;
  C.CO_1 = 5;

  C.T[0][0] = 2;
  C.T[0][1] = -1;

  C.T[1][0] = 3;
  C.T[1][1] = -1;

  //C.T[2][0] = 5;
  //C.T[2][1] = -1;
  */

  //complicated test cable
  {
  /*
  C.CI = 14;
  C.CO_1 = 21;
  C.CO_2 = 28;
  C.CO_3 = 43;
  C.tot_pins = 44;


  C.T[0][0] = 17;
  C.T[0][1] = 29;
  C.T[0][2] = -1;

  C.T[1][0] = 19;
  C.T[1][1] = 30;
  C.T[1][2] = -1;

  C.T[2][0] = 28;
  C.T[2][1] = 31;
  C.T[2][2] = -1;
  
  C.T[3][0] = 15;
  C.T[3][1] = 32;
  C.T[3][2] = -1;

  C.T[4][0] = 16;
  C.T[4][1] = 33;
  C.T[4][2] = -1;

  C.T[5][0] = 20;
  C.T[5][1] = 34;
  C.T[5][2] = -1;

  C.T[6][0] = 18;
  C.T[6][1] = 35;
  C.T[6][2] = -1;

  C.T[7][0] = 24;
  C.T[7][1] = 36;
  C.T[7][2] = -1;

  C.T[8][0] = 25;
  C.T[8][1] = 37;
  C.T[8][2] = -1;

  C.T[9][0] = -1;

  C.T[10][0] = 21;
  C.T[10][1] = 39;
  C.T[10][2] = -1;

  C.T[11][0] = 27;
  C.T[11][1] = 40;
  C.T[11][2] = -1;

  C.T[12][0] = 22;
  C.T[12][1] = 41;
  C.T[12][2] = -1;

  C.T[13][0] = 23;
  C.T[13][1] = 42;
  C.T[13][2] = -1;

  C.T[14][0] = 26;
  C.T[14][1] = 43;
  C.T[14][2] = -1;
  */
  }

  //Arduino Nano test cable
  C.tot_pins = 4;
  C.CI = 1;
  C.CO_1 = 3;
  C.CO_2 = 3;
  C.CO_3 = 3;

  C.T[0][0] = 2;
  C.T[0][1] = -1;

  C.T[1][0] = 3;
  C.T[1][1] = -1;
  
  SoftPWMBegin();
  SoftPWMSet(MASTER_ERROR, 0);
  SoftPWMSet(MASTER_GOOD, 0);
  //pinMode(MASTER_GOOD, OUTPUT);
  //digitalWrite(MASTER_GOOD, LOW);

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Cable Tester V1"));
  lcd.setCursor(0, 1);
  lcd.print(F("Baldean & Meza"));
  delay(1200);
  lcd.clear();

  Serial.print(digitalRead(BUTTON_OK));
}

void loop()
{
  SoftPWMSet(MASTER_GOOD, 0);
  SoftPWMSet(MASTER_ERROR, 0);
    
  err master_error_list;
  master_error_list.err_count = 0;
  
  //Default mode is fast
  State.mode = 0;

  if(reset_happened == 0)   //Operator should choose a mode
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Choose a mode:"));
    lcd.setCursor(0, 1);
    if(State.mode == 0)
    {
        lcd.print(F("Fast mode"));
    }
    while(digitalRead(BUTTON_OK) == 1)
    {
        if(digitalRead(BUTTON_NEXT) == 0)
        {
            State.mode++;
            if(State.mode == 3)
            {
                State.mode = 0;
            }
            switch(State.mode)
            {
                case 0:
                    lcd.setCursor(0, 1);
                    lcd.print(F("                "));
                    lcd.setCursor(0, 1);
                    lcd.print(F("Fast mode"));
                    break;
                case 1:
                    lcd.setCursor(0, 1);
                    lcd.print(F("                "));
                    lcd.setCursor(0, 1);
                    lcd.print(F("Stop on error"));
                    break;
                case 2:
                    lcd.setCursor(0, 1);
                    lcd.print(F("                "));
                    lcd.setCursor(0, 1);
                    lcd.print(F("Blind mode"));
                    break;
            }
            delay(700);
        }
    }
  }

  lcd.clear();
  reset_happened = 0;

  
  Serial.println(F("##### BEGIN CABLE TESTING ... #####"));
  int error_found = 0;

  //Begin calling check algorithm
  for(int in_pin = 0; in_pin <= C.CI; in_pin++)
  {
    //check pin
    err rep;
    if(State.mode == 0)
    {
        rep = check_pin(C, in_pin, State);
    }
    else
    {
        rep = check_pin(C, in_pin, 1);
    }
    
    if(rep.err_count > 0)
    {
      error_found = 1;
      concat_errors(&master_error_list, &rep);
      Serial.print(F("Concat_function_returned"));
    }
    if(reset_happened)
    {
      break;
    }
  }

  
  if(error_found == 0 && !reset_happened)
  {
//    digitalWrite(MASTER_GOOD, HIGH);
    SoftPWMSet(MASTER_GOOD, 5);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("WIRE OK"));
    lcd.setCursor(0, 1);
    lcd.print(F("Press RESET"));
    Serial.println(F("##### CABLE TESTING DONE -> CABLE OK (send r or R for rerun ... #####"));
    Serial.println();
  }
  else
  {
    if(reset_happened)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Reset pressed"));
        lcd.setCursor(0, 1);
        lcd.print(F("Resetting..."));
      Serial.println(F("!!!!! RESET BUTTON PRESSED. RESETING..."));
      Serial.println();
      delay(600);
    }
    else
    {
      Serial.println(F("##### CABLE TESTING DONE -> ERRORS FOUND (send r or R for rerun ... #####"));
      Serial.println();
      print_err_str_lcd(&master_error_list);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Errors found"));
      lcd.setCursor(0, 1);
      lcd.print(F("Press RESET"));
    }
  }


  //Waiting for input
  while(true)
  {
    if(reset_happened == 0)
    {
      reset_happened = !digitalRead(BUTTON_RESET);
    }
    char Redo = Serial.read();
    if(Redo == 'r' || Redo == 'R' || reset_happened)
    {
      while(Serial.read() >= 0)
      {
        ;
      }
      break;
    }
  }
}
