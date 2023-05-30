#include "SoftwareSerial.h";
#include "stdio.h"

int bluetoothTx = 2;
int bluetoothRx = 3;

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

char mystr[1000];
char* p;

void setup()
{
  Serial.begin(9600);

  bluetooth.begin(115200);
  bluetooth.print("$");
  bluetooth.print("$");
  bluetooth.print("$");
  delay(100);
  bluetooth.println("U,9600,N");
  bluetooth.begin(9600);
}

/*isr()
{
  short[i++] = analog.read(A0);
  puls = analog.read(A1);
  DHT11.read();
}*/

// constant ce receptioneaza pe bluetooth -> comanda specifica pentru senzori e.g. puls, temperatura
// in cazul in care Arduino primeste comanda -> procesul de citire de senzori
// proces de citire la 10 s

void loop()
{
  sprintf(mystr, "{\"ecg\":{\"P\":{\"durata\":%d}}}\n", 15);
  p = mystr;
  if(bluetooth.available()) {
    char toSend = (char)bluetooth.read();
    while(*p != NULL)
    {
      Serial.print(*p);
      p++;
    }
  }
 // "{blabla:\"asdasd\""}"
  if(Serial.available()) {
    char toSend = (char)Serial.read();
    bluetooth.print((char)toSend);
  }
}