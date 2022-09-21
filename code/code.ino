#define MAX_ERRORS 10
#define MASTER_GOOD A14
#define MASTER_ERROR A15
#define BUTTON_NEXT A8
#define BUTTON_RESET A9

#define RED_BRIGHTNESS 70

#include <SoftPWM.h>        //by Brett Hagman
#include <LiquidCrystal_I2C.h>

//Declare global variables
int reset_happened = 0;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// available pins in this array
int HW_P[] = {2, 3, 4, 5, 6, 7, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, A0, A1, A2, A3, A4, A5, A6, A7};
//            0  1  2  3  4  5  6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  41  42  43  44

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

err check_pin(cable C, const int i, const int SOE = 1)
{
  //TO DO
  //Check if i belongs to the input connector
  err report;
  report.err_count = 0;

  //Make sure MASTER_GOOD LED is NOT on when testing
  digitalWrite(MASTER_GOOD, LOW);

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

void print_S(err report, int current_in_pin)
{
  int i = 0;
  Serial.print("Number of errors on in_pin ");
  Serial.print(current_in_pin);
  Serial.print(": ");
  Serial.println(report.err_count);

  while(i < report.err_count && i < MAX_ERRORS)
  {
    Serial.print(report.err_list[i][0]);
    Serial.print(" ");
    Serial.print(report.err_list[i][1]);
    Serial.print(" ");
    Serial.println(report.err_list[i][2]);
    i++;
  }
}

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file
cable C;

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
  

  SoftPWMBegin();
  SoftPWMSet(MASTER_ERROR, 0);
  pinMode(MASTER_GOOD, OUTPUT);
  digitalWrite(MASTER_GOOD, LOW);

  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);

  Serial.begin(9600);
}

void loop()
{
  reset_happened = 0;
  
  Serial.println("##### BEGIN CABLE TESTING ... #####");
  int error_found = 0;

  //Begin calling check algorithm
  for(int in_pin = 0; in_pin <= C.CI; in_pin++)
  {
    err rep = check_pin(C, in_pin);
    if(rep.err_count > 0)
    {
      error_found = 1;
    }
    if(reset_happened)
    {
      break;
    }
  }

  
  if(error_found == 0 && !reset_happened)
  {
    digitalWrite(MASTER_GOOD, HIGH);
    Serial.println("##### CABLE TESTING DONE -> CABLE OK (send r or R for rerun ... #####");
    Serial.println();
  }
  else
  {
    if(reset_happened)
    {
      Serial.println("!!!!! RESET BUTTON PRESSED. RESETING...");
      Serial.println();
    }
    else
    {
      Serial.println("##### CABLE TESTING DONE -> ERRORS FOUND (send r or R for rerun ... #####");
      Serial.println();
    }
  }

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

  digitalWrite(MASTER_GOOD, LOW);
  //digitalWrite(MASTER_ERROR, LOW);
  SoftPWMSet(MASTER_ERROR, 0);
}
