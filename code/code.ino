//46 available pins in this array
int HW_P[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};

struct cable
{
  int tot_pins;
  int CI = -1;        //Last sequential pin number of INPUT CONNECTOR
  int CO_1 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 1
  int CO_2 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 2
  int CO_3 = -1;    //Last sequential pin number of OUTPUT CONNECTOR 3

  int T[100][100];
};

struct err
{
  int correct_ok = 0;
  int open_c = 0;
  int X = 0;
  int Y = 0;
  int short_c = 0;
};

err check_pin(cable C, int i)
{
  err report;
  report.correct_ok = 1;

  pinMode(HW_P[i], OUTPUT);
  digitalWrite(HW_P[i], HIGH);

  int j = 0;
  while(C.T[i][j] != -1)  //Check correct wires
  {
    if(digitalRead(HW_P[C.T[i][j]]) != HIGH)
    {
      report.correct_ok = 0;
    }
    j++;
  }
  
  for(int j = 0; j < C.tot_pins; j++)   //Check the rest of the wires
  {
    //Only check incorrect wires (correct ones have allready been tested)
    int incorrect = 0;
    int k = 0;
    while(C.T[i][k] != -1)
    {
      if (j != C.T[i][k])
      {
        incorrect = 1;
      }
      k++;
    }
    if(j == i)
    {
      incorrect = 1;
    }

    if(incorrect)   //Checking wires that are incorrect
    {
      //check for short circuit errors
      if(j < C.CI && digitalRead(HW_P[C.T[i][j]]) == HIGH)
      {
        report.short_c++;
      }
      if(j >= C.CI && j < C.CO_1 && digitalRead(HW_P[C.T[i][j]]) == HIGH)
      {
        report.X++;
      }
    }
  }

  //Put current pin back to INPUT and Return results
  pinMode(HW_P[i], INPUT);
  return report;
}

//Declare a cable variable and initialize it with data as described in the cable_template_format.txt file

void setup()
{
  
}

void loop()
{
  
}
