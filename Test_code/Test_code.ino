int HW_P[] = {2, 3, 4, 5, 6, 7, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, A0, A1, A2, A3, A4, A5, A6, A7};


void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.println(sizeof(HW_P));
  
  for(int i = 0; i < 46; i++)
  {
    pinMode(HW_P[i], OUTPUT);
    digitalWrite(HW_P[i], HIGH);
    delay(600);
    digitalWrite(HW_P[i], LOW);
    pinMode(HW_P[i], INPUT);
  }

  while(1)
  {
    ;
  }
}
