#define MAX_ERRORS 10       //Max number of errors that can be stored in the list of errors
#define MASTER_GOOD A14     //Master good LED port  (turns on when all test completed without errors)
#define MASTER_ERROR A15    //Master error LED port (turns on on any error)
#define BUTTON_NEXT A8      //"Next" Button Port
#define BUTTON_RESET A9     //"Reset" Button Port
#define BUTTON_OK A10       //"OK" Button Port
#define POT_LED_SPEED A11   //Port for the led speed controll potentiometer

#define RED_BRIGHTNESS 150   //Controls LED brightness
#define GREEN_BRIGHTNESS 10   //Controls LED brightness

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

struct hardware_model
{
  //Containes a description of present hardware options
  bool lcd; //true if LCD present
  int board;//:0 - arduino mega, 1 - arduino uno, ...
};

//Declare global variables
int reset_happened = 1;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file
cable C;
//Declare a modes variable for user interface info
modes State;
//Declare a hardware_model configuration
hardware_model Hardware;

// available pins in this array
int HW_P[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};
////          0  1  2  3  4  5  6  7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43

//For arduino nano
//int HW_P[] = {2, 3, 5, 6, 7, 8, 9, 10, 11};
//              0  1  2  3  4  5  6  7   8   9   10  11
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
  delayMicroseconds(100);  //Wait to settle

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


void save_golden_standard()
{
    Serial.println(F("Begin golden sample reading..."));
    char max_id = 0;
    if(Hardware.board == 0)
        max_id = 43;
    else if(Hardware.board == 1)
        max_id = 8;
    else max_id = 8;

    //Set all pins to input low
    for(int i = 0; i <= max_id; i++)
    {
        pinMode(HW_P[i], INPUT);
        digitalWrite(HW_P[i], LOW);
    }
    //turn master good and master error leds off
    SoftPWMSet(MASTER_GOOD, 0);
    SoftPWMSet(MASTER_ERROR, 0);
        
    char max_input_connector_id = 0;
    char min_output_connector_id = max_id;
    char max_used_id = 0;

    for(int i = 0; i <= max_id && max_input_connector_id == 0; i++)
    {
        digitalWrite(HW_P[i], HIGH);
        pinMode(HW_P[i], OUTPUT);
        delay(1);    //maybe wait for residual capacitances to charge
        int recipients_found = 0;
        for(int j = 0; j <= max_id; j++)
        {
            if(j != i)  //jump over current pin
            {
                if(digitalRead(HW_P[j]) == 1)
                {
                    C.T[i][recipients_found] = j;
                    recipients_found++;
                    if(j < min_output_connector_id)
                        min_output_connector_id = j;
                    if(j > max_used_id)
                        max_used_id = j;

                    Serial.print(i);
                    Serial.print(F(" "));
                    Serial.println(j);
                }
            }
        }
        //end recipient sequence
        C.T[i][recipients_found] = -1;
        
        //Conditions for end of input connector
        if(recipients_found == 0)
            max_input_connector_id = i - 1;
        if(max_input_connector_id == -1)
            max_input_connector_id = 0;
        if(i == min_output_connector_id)
            max_input_connector_id = i - 1;

        delay(State.led_speed);
        pinMode(HW_P[i], INPUT);
        digitalWrite(HW_P[i], LOW);
    }
    C.CI = max_input_connector_id;
    C.CO_1 = max_used_id;
    C.CO_2 = max_used_id;
    C.CO_3 = max_used_id;
    C.tot_pins = max_used_id + 1;

    //in case there is no cable connected while sampling golden standard
    if(C.tot_pins == 1)
        C.T[0][0] = -1;

    //turn master good and master error leds back on
    SoftPWMSet(MASTER_GOOD, GREEN_BRIGHTNESS);
    SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
    Serial.println(F("Cable saved as Golden Sample"));
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
  /*
  {
  ///*
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
  }
  //*/

  //Arduino Nano test cable
  /*
  C.tot_pins = 4;
  C.CI = 1;
  C.CO_1 = 3;
  C.CO_2 = 3;
  C.CO_3 = 3;

  C.T[0][0] = 2;
  C.T[0][1] = -1;

  C.T[1][0] = 3;
  C.T[1][1] = -1;
  */

  //Demo cable
  C.CI = 4;
  C.CO_1 = 33;
  C.CO_2 = 33;
  C.CO_3 = 33;
  C.tot_pins = 34;
  
  C.T[0][0] = 29;
  C.T[0][1] = -1;

  C.T[1][0] = 30;
  C.T[1][1] = -1;

  C.T[2][0] = 31;
  C.T[2][1] = -1;

  C.T[3][0] = 32;
  C.T[3][1] = -1;

  C.T[4][0] = 33;
  C.T[4][1] = -1;

  //Used hardware configuration
  Hardware.lcd = false;
  Hardware.board = 0;
  
  
  SoftPWMBegin();
  SoftPWMSet(MASTER_ERROR, 0);
  SoftPWMSet(MASTER_GOOD, 0);

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);

  Serial.begin(115200);

  //Default mode is fast
  State.mode = 1;

  if(Hardware.lcd == true)
  {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(F("Cable Tester V1"));
    lcd.setCursor(0, 1);
    lcd.print(F("Baldean"));
    delay(1200);
    lcd.clear();
  }

  Serial.print(digitalRead(BUTTON_OK));
}

void loop()
{
  SoftPWMSet(MASTER_GOOD, 0);
  SoftPWMSet(MASTER_ERROR, 0);
    
  err master_error_list;
  master_error_list.err_count = 0;

  if(reset_happened == 1)   //Operator should choose a mode
  {
    unsigned long int last_time = millis();
    unsigned long int last_blink = millis();
    
    digitalWrite(HW_P[0], LOW);
    digitalWrite(HW_P[1], LOW);
    digitalWrite(HW_P[2], LOW);
    SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
    SoftPWMSet(MASTER_GOOD, GREEN_BRIGHTNESS);
    
    if(Hardware.lcd == true)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Choose a mode:"));
      lcd.setCursor(0, 1);
    }
    Serial.print(F("\nChoose a mode:\n"));
    
    if(State.mode == 1)
    {
        if(Hardware.lcd == true) lcd.print(F("Fast mode"));
        Serial.print(F("Fast mode\n"));

        pinMode(HW_P[0], OUTPUT);
        pinMode(HW_P[1], INPUT);
        pinMode(HW_P[2], INPUT);
        digitalWrite(HW_P[0], HIGH);
        digitalWrite(HW_P[1], LOW);
        digitalWrite(HW_P[2], LOW);
    }
    if(State.mode == 2)
    {
        if(Hardware.lcd == true) lcd.print(F("Stop on error"));
        Serial.print(F("Stop on error\n"));

        pinMode(HW_P[0], INPUT);
        pinMode(HW_P[1], OUTPUT);
        pinMode(HW_P[2], INPUT);
        digitalWrite(HW_P[0], LOW);
        digitalWrite(HW_P[1], HIGH);
        digitalWrite(HW_P[2], LOW);
    }
    if(State.mode == 3)
    {
        if(Hardware.lcd == true) lcd.print(F("Save as GS"));
        Serial.print(F("Save as Golden Sample\n"));

        pinMode(HW_P[0], INPUT);
        pinMode(HW_P[1], INPUT);
        pinMode(HW_P[2], OUTPUT);
        digitalWrite(HW_P[0], LOW);
        digitalWrite(HW_P[1], LOW);
        digitalWrite(HW_P[2], HIGH);
    }

    int i = 29; //Incrementing variable for displaying led speed
    int wait = 0;
    while(digitalRead(BUTTON_OK) || wait == 1)
    {
        //Reading "Next" button presses
        if(digitalRead(BUTTON_NEXT) == 0 && millis() - last_time > 700)
        {
            State.mode++;
            if(State.mode == 4)
            {
                State.mode = 1;
            }
            switch(State.mode)
            {
                case 1:
                    State.mode = 1;
                    if(Hardware.lcd == true)
                    {
                        lcd.setCursor(0, 1);
                        lcd.print(F("                "));
                        lcd.setCursor(0, 1);
                        lcd.print(F("Fast mode"));
                    }
                    Serial.print(F("Fast mode\n"));
                    pinMode(HW_P[0], OUTPUT);
                    pinMode(HW_P[1], INPUT);
                    pinMode(HW_P[2], INPUT);
                    digitalWrite(HW_P[0], HIGH);
                    digitalWrite(HW_P[1], LOW);
                    digitalWrite(HW_P[2], LOW);
                    break;
                case 2:
                  State.mode = 2;
                  if(Hardware.lcd == true)
                  {
                        lcd.setCursor(0, 1);
                        lcd.print(F("                "));
                        lcd.setCursor(0, 1);
                        lcd.print(F("Stop on error"));
                  }
                  Serial.print(F("Stop on error\n"));
                  pinMode(HW_P[0], INPUT);
                  pinMode(HW_P[1], OUTPUT);
                  pinMode(HW_P[2], INPUT);
                  digitalWrite(HW_P[0], LOW);
                  digitalWrite(HW_P[1], HIGH);
                  digitalWrite(HW_P[2], LOW);
                    break;
                case 3:
                  State.mode = 3;
                  if(Hardware.lcd == true)
                  {
                    lcd.setCursor(0, 1);
                    lcd.print(F("                "));
                    lcd.setCursor(0, 1);
                    lcd.print(F("Save cable as GS"));
                  }
                  Serial.print(F("Save cable as Golden Sample\n"));
                  pinMode(HW_P[0], INPUT);
                  pinMode(HW_P[1], INPUT);
                  pinMode(HW_P[2], OUTPUT);
                  digitalWrite(HW_P[0], LOW);
                  digitalWrite(HW_P[1], LOW);
                  digitalWrite(HW_P[2], HIGH);
                  break;
            }
            last_time = millis();
        }

        //Reading Led speed potentiometer
        State.led_speed = map(analogRead(POT_LED_SPEED), 0, 255, 5, 50);
        //for(int i = 29, i < 44, i++)
        if(millis() - last_blink > State.led_speed)
        {
            pinMode(HW_P[i], INPUT);
            digitalWrite(HW_P[i], LOW);
            i++;
            if(i >= 44) i = 29;
            digitalWrite(HW_P[i], HIGH);
            pinMode(HW_P[i], OUTPUT);
            last_blink = millis();
        }

        //Read and Update golden sample
        if(State.mode == 3)
        {
            wait = 1;
            if(digitalRead(BUTTON_OK) == 0)
            {
                save_golden_standard();
                delay(500);
                //turn mode led back on
                pinMode(HW_P[0], INPUT);
                pinMode(HW_P[1], INPUT);
                pinMode(HW_P[2], OUTPUT);
                digitalWrite(HW_P[0], LOW);
                digitalWrite(HW_P[1], LOW);
                digitalWrite(HW_P[2], HIGH);
            }
        }
        else wait = 0;
    }
    
    pinMode(HW_P[i], INPUT);
    digitalWrite(HW_P[i], LOW);
    
    pinMode(HW_P[0], INPUT);
    pinMode(HW_P[1], INPUT);
    pinMode(HW_P[2], INPUT);
    digitalWrite(HW_P[0], LOW);
    digitalWrite(HW_P[1], LOW);
    digitalWrite(HW_P[2], LOW);
    SoftPWMSet(MASTER_ERROR, 0);
    SoftPWMSet(MASTER_GOOD, 0);
  }

  if(Hardware.lcd == true) lcd.clear();
  reset_happened = 0;

  
  Serial.println(F("##### BEGIN CABLE TESTING ... #####"));
  int error_found = 0;

  //Begin calling check algorithm
  for(int in_pin = 0; in_pin <= C.CI; in_pin++)
  {
    //check pin
    err rep;
    rep = check_pin(C, in_pin, State);
    
    if(rep.err_count > 0)
    {
      error_found = 1;
      concat_errors(&master_error_list, &rep);
      //Serial.print(F("Concat_function_returned"));
    }
    if(reset_happened)
    {
      break;
    }
  }

  
  if(error_found == 0 && !reset_happened)
  {
//    digitalWrite(MASTER_GOOD, HIGH);
    SoftPWMSet(MASTER_GOOD, GREEN_BRIGHTNESS);
    //if cable template is empty blink the green led a few times
    if(C.tot_pins == 1)
    {
        Serial.println(F("Cable template is empty!!!"));
        for(int i = 0; i < 5; i++)
        {
            SoftPWMSet(MASTER_GOOD, GREEN_BRIGHTNESS);
            delay(250);
            SoftPWMSet(MASTER_GOOD, 0);
            delay(250);
        }
    }
    if(Hardware.lcd == true)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("WIRE OK"));
      lcd.setCursor(0, 1);
      lcd.print(F("Press RESET"));
    }
    if(C.tot_pins == 1)
        Serial.print(F("##### CABLE TESTING DONE -> NO TEMPLATE (send r or R for rerun ... #####"));
    else
    Serial.println(F("##### CABLE TESTING DONE -> CABLE OK (send r or R for rerun ... #####"));
    Serial.println();
  }
  else
  {
    if(reset_happened)
    {
      if(Hardware.lcd == true)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Reset pressed"));
        lcd.setCursor(0, 1);
        lcd.print(F("Resetting..."));
      }
      Serial.println(F("!!!!! RESET BUTTON PRESSED. RESETING..."));
      Serial.println();
      delay(600);
    }
    else
    {
      Serial.println(F("##### CABLE TESTING DONE -> ERRORS FOUND (send r or R for rerun ... #####"));
      Serial.println();
      
      if(Hardware.lcd == true)
      {
          print_err_str_lcd(&master_error_list);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(F("Errors found"));
          lcd.setCursor(0, 1);
          lcd.print(F("Press RESET"));
      }
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
    if(Redo == 'r' || Redo == 'R' || reset_happened || digitalRead(BUTTON_OK) == 0)
    {
      while(Serial.read() >= 0)
      {
        ;
      }
      break;
    }
  }
}
