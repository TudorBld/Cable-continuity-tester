
#include <SoftPWM.h>        //by Brett Hagman
//https://github.com/bhagman/SoftPWM  V1.0.1
#include <LiquidCrystal_I2C.h>
//https://github.com/johnrickman/LiquidCrystal_I2C  V1.1.2
#include <EEPROM.h>

#include "Macros.h"
#include "Core_structs.h"
#include "Data_manipulation_functions.h"

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


void save_golden_standard_V2()
{
    Serial.println(F("BEGIN READING GOLDEN SAMPLE"));
    
    //save total number of available pins depending on board model
    int k = 0;
    switch(Hardware.board)
    {
        case 0:
            k = 44;
            break;
        case 1:
            k = 9;
    }
    //Create the frequency vector
    char V[k];
    for(int i = 0; i < k; i++)
    {
        V[i] = 0;
    }

    cable GS;
    //Initialise the new template as empty
    for(int p = 0; p < MAX_ROOTS; p++)
    {
        for(int pp = 0; pp < MAX_WIRES_IN_NET; pp++)
        {
            GS.T[p][pp] = -1;
        }
    }

    //Set all pins to input low
    for(int i = 0; i < k; i++)
    {
        pinMode(HW_P[i], INPUT);
        digitalWrite(HW_P[i], LOW);
    }
    //turn master good and master error leds off
    SoftPWMSet(MASTER_GOOD, 0);
    SoftPWMSet(MASTER_ERROR, 0);

    int max_root = 0;   //ID of the last root (in the HW_P order)
    for(int i = 0; i < k; i++)
    {
        digitalWrite(HW_P[i], HIGH);
        pinMode(HW_P[i], OUTPUT);

        if(V[i] == 0)   //Check if current pin already belongs to a net
        {
            int link_nr = 0;
            for(int j = i + 1; j < k; j++)
            {
                if(digitalRead(HW_P[j]) == 1 && V[j] == 0)
                {
                    if(link_nr == 0)
                    {
                        max_root = i;
                    }
                    if(link_nr < MAX_WIRES_IN_NET)
                    {
                        GS.T[i][link_nr] = j;
                        link_nr ++;
                        V[j] = 1;
                        Serial.print(i);
                        Serial.print(F(" "));
                        Serial.println(j);
                    }
                    else
                    {
                        Serial.println(F("ERROR!...TOO MANY WIRES CONNECTED TOGETHER!"));
                        if(Hardware.lcd == true)
                        {
                            lcd.setCursor(0, 0);
                            lcd.print(F("ERROR!"));
                            lcd.setCursor(0, 1);
                            lcd.print(F("READ INCORRECT"));
                        }
                    }
                }
            }
        }
        
        pinMode(HW_P[i], INPUT);
        digitalWrite(HW_P[i], LOW);
        V[i] = 1;
    }
    //Check if the GS.T is empty
    if(max_root != 0 || GS.T[0][0] != -1)
    {
        GS.tot_pins = k;
        GS.CI = max_root;
        GS.CO_1 = GS.CO_2 = GS.CO_3 = k - 1;

        //sss
        C = GS;
        
        // save cable template on EEPROM;
        EEPROM.write(0, CABLE_PRESENT);
        EEPROM.put(1, GS);
        Serial.println(F("GS SAVED on EEPROM"));
    }
    else
    {
        Serial.println(F("READED CABLE IS EMPTY! READ IS NOT SAVED!"));
    }
    
    //turn master good and master error leds back on
    SoftPWMSet(MASTER_GOOD, GREEN_BRIGHTNESS);
    SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
}

void save_golden_sample_V3()
{
  Serial.println("start reading golden sample...");

  // k is the mak number of pins available
  int k = 0;
    switch(Hardware.board)
    {
        case 0:
            k = 44;
            break;
        case 1:
            k = 9;
    }
    if(k > MAX_ROOTS) k = MAX_ROOTS;  // We cannot exceed the max number of pins available, or the max number of rows available

    //Initialise the new template as empty
    cable GS;
    for(int p = 0; p < MAX_ROOTS; p++)
    {
        for(int pp = 0; pp < MAX_WIRES_IN_NET; pp++)
        {
            GS.T[p][pp] = -1;
        }
    }

    // Stop OK and ERROR leds
    for(int i = 0; i <= k; i++)
    {
      pinMode(HW_P[i], INPUT);
      digitalWrite(HW_P[i], LOW);
    }

    SoftPWMSet(MASTER_GOOD, 0);
    SoftPWMSet(MASTER_ERROR, 0);

    for(int i = 0; i < k; i++) // For each available pin
    {
      digitalWrite(HW_P[i], HIGH);
      pinMode(HW_P[i], OUTPUT);

      int found = 0;
      for(int j = 0; j < k; j++) // For each available pin
      {
        if(j != i)  // Jump over the curent root
        {
          if(digitalRead(HW_P[j]) == HIGH)  // Found a return wire
          {
            if(found > MAX_WIRES_IN_NET)  // If we reached the max number of wires in a net
            {
              Serial.print("TOO MANY WIRES IN NET ");
              Serial.println(i);
            }
            else  // Add the return pin to the root's list
            {
              GS.T[i][found] = j;
              found++;
            }
          }
        }
      }

      // set current pin too low
      pinMode(HW_P[i], INPUT);
      digitalWrite(HW_P[i], LOW);
    }

    // Check if any cable is connected
    int emphty = 1;
    for(int i = 0; i < k; i++)
    {
      if(GS.T[i][0] != -1) emphty = 0;
    }

    if(emphty == 1)
    {
      Serial.print(F("NO CABLE PRESENT!!! TEMPLATE NOT SAVED!"));
    }
    else
    {
      GS.tot_pins = k;
      GS.CI = k;
      GS.CO_1 = GS.CO_2 = GS.CO_3 = k - 1;

      //sss
      C = GS;
      
      // save cable template on EEPROM;
      EEPROM.write(0, CABLE_PRESENT);
      EEPROM.put(1, GS);
      Serial.println(F("GS SAVED on EEPROM"));

      for(int ii = 0; ii < MAX_ROOTS; ii++)
      {
        for(int jj = 0; jj < MAX_WIRES_IN_NET; jj++)
        {
          Serial.print(C.T[ii][jj]);
          Serial.print(" ");
        }
        Serial.println();
      }
    }
}

void setup()
{
  //Initializing the cable as empthy
  for(int ii = 0; ii < MAX_ROOTS; ii++)
 {
    for(int jj = 0; jj < MAX_WIRES_IN_NET; jj++)
    {
        C.T[ii][jj] = -1;
    }
 }

  //Initializing serial communication
  Serial.begin(115200);

  if(EEPROM.read(0) == CABLE_PRESENT)
  {
    Serial.print(F("CABLE TEMPLATE FOUND IN EEPROM. READING ..."));
    EEPROM.get(1, C);
    Serial.println(F("DONE"));
  }
  else
  {
    Serial.print(F("NO CABLE TEMPLATE FOUND IN EEPROM. LOADING DEMO CABLE ... DONE"));
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
  }


  //Used hardware configuration
  Hardware.lcd = false;
  Hardware.board = 0;
  
  
  SoftPWMBegin();
  SoftPWMSet(MASTER_ERROR, 0);
  SoftPWMSet(MASTER_GOOD, 0);

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);


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

  //Serial.print(digitalRead(BUTTON_OK));
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
    Serial.print(F("\nChoose a mode: (send 'n' for next option or 's' for selecting)\n"));
    
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
    char command = 0;
    while( (digitalRead(BUTTON_OK) && command != 's') || wait == 1)
    {
        command = Serial.read();
        
        //Reading "Next" button presses
        if( (digitalRead(BUTTON_NEXT) == 0 || command == 'n') && millis() - last_time > 700)
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
            //Empty Serial input
            while(Serial.read() >= 0)
            {
                ;
            }
        }

        //Reading Led speed potentiometer
        State.led_speed = map(analogRead(POT_LED_SPEED), 0, 255, 5, 150);
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
            if(digitalRead(BUTTON_OK) == 0 || command == 's')
            {
                save_golden_sample_V3();
                delay(500);
                //turn mode led back on
                pinMode(HW_P[0], INPUT);
                pinMode(HW_P[1], INPUT);
                pinMode(HW_P[2], OUTPUT);
                digitalWrite(HW_P[0], LOW);
                digitalWrite(HW_P[1], LOW);
                digitalWrite(HW_P[2], HIGH);

                //Empty Serial input
                while(Serial.read() >= 0)
                {
                    ;
                }
            }
        }
        else wait = 0;
    }
    //Empty Serial input
    while(Serial.read() >= 0)
    {
        ;
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

  //DEBUG - show the whole cable template
  //for(int ii = 0; ii < MAX_ROOTS; ii++)
  //{
  //  for(int jj = 0; jj < MAX_WIRES_IN_NET; jj++)
  //  {
  //      Serial.print(C.T[ii][jj]);
  //      Serial.print(" ");
  //  }
  //  Serial.println();
  //}

  //Begin calling check algorithm on wires that are inputs (roots)
  int in_pin = 0;
  int ii = 0;
  
  //for(int in_pin = 0; in_pin <= C.CI; in_pin++)
  // for each pin that is root (input wire), we run the check function
  while(C.T[i][0] != -1 && i < MAX_ROOTS)
  {
    //DEBUG
    //Serial.print("checked pin: ");
    //Serial.println(ii);
    
    in_pin = i;
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
    i++;
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
        Serial.print(F("##### CABLE TESTING DONE -> NO TEMPLATE (send r for rerun, or x to return to menu ...) #####"));
    else
    Serial.println(F("##### CABLE TESTING DONE -> CABLE OK (send r for rerun, or x to return to menu ...) #####"));
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
      Serial.println(F("##### CABLE TESTING DONE -> ERRORS FOUND (send r or R for rerun, or x to return to menu ...) #####"));
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
    char command = Serial.read();
    
    if(reset_happened == 0)
    {
      reset_happened = !digitalRead(BUTTON_RESET);
      if(command == 'x' || command == 'X')
        reset_happened = 1;
        while(Serial.read() >= 0)
        {
          ;
        }
        
        if(reset_happened == 1)
        {
          break;
        }
    }
    
    if(command == 'r' || command == 'R' || reset_happened || digitalRead(BUTTON_OK) == 0)
    {
      while(Serial.read() >= 0)
      {
        ;
      }
      break;
    }
  }
}
