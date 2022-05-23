#define MAX_ERRORS 10

//46 available pins in this array
int HW_P[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};

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

err check_pin(cable C, const int i)
{
  //TO DO
  //Check if i belongs to the input connector
  err report;
  report.err_count = 0;

  pinMode(HW_P[i], OUTPUT);
  digitalWrite(HW_P[i], HIGH);
  delay(1000);

  int j = 0;
  while(C.T[i][j] != -1)  //Check correct wires
  {
    if(digitalRead(HW_P[C.T[i][j]]) == LOW)
    {
      if(report.err_count < MAX_ERRORS)
      {
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
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 3;          //error ID (in this case short circuit)
        report.err_list[report.err_count][2] = j;  //destination pin
        report.err_count ++;
      }
      //check for mismatch errors
      if(j > C.CI && j <= C.CO_1 && digitalRead(HW_P[j]) == HIGH)
      {
        report.err_list[report.err_count][0] = i;          //ID of faulty pin
        report.err_list[report.err_count][1] = 1;          //error ID (in this case mismatch)
        report.err_list[report.err_count][2] = j;  //destination pin
        report.err_count ++;
      }
    }
  }

  //Put current pin back to INPUT and Return results
  digitalWrite(HW_P[i], LOW);
  pinMode(HW_P[i], INPUT);
  delay(1);
  return report;
}

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file
cable C;

void setup()
{
  //Initializing the cable
  C.tot_pins = 4;
  C.CI = 1;
  C.CO_1 = 5;

  C.T[0][0] = 2;
  C.T[0][1] = -1;

  C.T[1][0] = 3;
  C.T[1][1] = -1;

  //C.T[2][0] = 5;
  //C.T[2][1] = -1;

  Serial.begin(9600);
}

void loop()
{
  Serial.println("##### BEGIN CABLE TESTING ... #####");
  for(int in_pin = 0; in_pin <= C.CI; in_pin++)
  {
    err rep = check_pin(C, in_pin);
    int i = 0;
    Serial.print("Number of errors on in_pin ");
    Serial.print(in_pin);
    Serial.print(": ");
    Serial.println(rep.err_count);
    while(i < rep.err_count && i < MAX_ERRORS)
    {
      Serial.print(rep.err_list[i][0]);
      Serial.print(" ");
      Serial.print(rep.err_list[i][1]);
      Serial.print(" ");
      Serial.println(rep.err_list[i][2]);
      i++;
    }
  }
  Serial.println("##### CABLE TESTING DONE (send r or R for rerun ... #####");
  Serial.println();

  while(true)
  {
    char Redo = Serial.read();
    if(Redo == 'r' || Redo == 'R')
    {
      while(Serial.read() >= 0)
      {
        ;
      }
      break;
    }
  }
}
