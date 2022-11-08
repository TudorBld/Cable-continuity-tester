#define MAX_ERRORS 10
#define MASTER_GOOD A0
#define MASTER_ERROR A1
#define BUTTON_NEXT A2
#define BUTTON_RESET A3
#define BUTTON_OK 12

#define RED_BRIGHTNESS 70

#include <SoftPWM.h>        //by Brett Hagman
#include <LiquidCrystal_I2C.h>

//STRUCTURES
struct modes
{
    byte mode;    //1 fast, 2 stop on error, 3 blind
};

struct cable
{
  int tot_pins;     //Total number of pins
  int CI = -1;      //Last sequential pin number of INPUT CONNECTOR
  int CO_1 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 1
  int CO_2 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 2
  int CO_3 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 3

  int T[50][5];
};

struct err
{
  int err_count = 0;
  //int open_c = 0;
  //int X = 0;
  //int Y = 0;
  //int short_c = 0;
  int err_list[MAX_ERRORS][3];
};


    //Declare global variables
int reset_happened = 0;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file
cable C;
//Declare a modes variable for user interface info
modes State;

// available pins in this array
//int HW_P[] = {2, 3, 4, 5, 6, 7, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, A0, A1, A2, A3, A4, A5, A6, A7};
////            0  1  2  3  4  5  6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  41  42  43  44

//For arduino nano
int HW_P[] = {2, 3, 5, 6, 7, 8, 9, 10, 11};
//            0  1  2  3  4  5  6  7   8   9   10  11


void concat_errors(err *master, err *addition)
{
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

err check_pin(cable C, const int i, const int SOE = 1)
{
  //TO DO
  //Check if i belongs to the input connector
  err report;
  report.err_count = 0;

  //Make sure MASTER_GOOD LED is NOT on when testing
  //digitalWrite(MASTER_GOOD, LOW);
  SoftPWMSet(MASTER_GOOD, 0);

  pinMode(HW_P[i], OUTPUT);
  digitalWrite(HW_P[i], HIGH);
  delay(10);  //Wait for some capacitor to charge?

  int j = 0;
  while(C.T[i][j] != -1)  //Check correct wires
  {
    if(digitalRead(HW_P[C.T[i][j]]) == LOW)
    {
      if(report.err_count < MAX_ERRORS)
      {
        //digitalWrite(MASTER_ERROR, HIGH);
        SoftPWMSet(MASTER_ERROR, RED_BRIGHTNESS);
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 0;          //error ID (in this case open circuit)
        report.err_list[report.err_count][2] = C.T[i][j];  //destination pin
        report.err_count ++;
      }
    }
    j++;
  }
  
  for(j = 0; j < C.tot_pins; j++)   //Check the rest of the wires
  {
    //Only check incorrect wires (correct ones have already been tested)
    int incorrect = 1;
    int k = 0;
    while(C.T[i][k] != -1)  //go trough the current template line
    {
      if (j == C.T[i][k])
      {
        incorrect = 0;
      }
      k++;
    }
    if(j == i)
    {
      incorrect = 0;
    }

    if(incorrect == 1)   //Checking wires that are incorrect
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
  //Visual LED debugging
  delay(1);

  print_S(report, i);

  //Stop on error if required
  if(SOE == 1 && report.err_count > 0)
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

void print_S(err report, int current_in_pin)
{
  int i = 0;
  Serial.print(F("Number of errors on in_pin "));
  Serial.print(current_in_pin);
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
        rep = check_pin(C, in_pin, 0);
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
