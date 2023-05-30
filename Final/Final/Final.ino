#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math
#define DHTPIN 7
#define DHTTYPE DHT11
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library
#include "SoftwareSerial.h";           // Used for Bluetooth Mate
#include <DHT.h>                     // DHT11

/*  JSON stuff  */
//unsigned char StrJSON[8066];
String StrJSON;
//unsigned short StrJSONIndex;

/*  DHT11  */
DHT dht(DHTPIN, DHTTYPE);


/* Pulse Sensor */
const int PulseWire = 1;       // 'S' Signal pin connected to A0
//const int LED13 = 13;          // The on-board Arduino LED
int Threshold = 550;           // Determine which Signal to "count as a beat" and which to ignore
int LastBPMRecorded = 0;
                               
/* Bluetooth sensor */
int bluetoothTx = 2;
int bluetoothRx = 3;

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

/**/

PulseSensorPlayground pulseSensor;  // Creates an object

void setup() {
  Serial.begin(9600);

  /* JSON */
  //memset(StrJSON, 0, 8066);
  //StrJSONIndex = 0;
  //strcpy(StrJSON, "{\"ecg\":\"")
  StrJSON = "{\"val_senzor_ecg\":\"";

  /* DHT */
  dht.begin();

  /* Bluetooth sensor config */
  bluetooth.begin(115200);
  bluetooth.print("$");
  bluetooth.print("$");
  bluetooth.print("$");
  delay(100);
  bluetooth.println("U,9600,N");
  bluetooth.begin(9600);

  /* ECG */
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT); // Setup for leads off detection LO -
  
	// Configure the PulseSensor object, by assigning our variables to it
	pulseSensor.analogInput(PulseWire);   
	//pulseSensor.blinkOnPulse(LED13);       // Blink on-board LED with heartbeat
	pulseSensor.setThreshold(Threshold);   

	// Double-check the "pulseSensor" object was created and began seeing a signal
	if (!pulseSensor.begin()) {
		Serial.println("Error creating PulseSensor object created!");
	}
}

char byteToAscii(unsigned char byte)
{
  switch(byte)
  {
    case 0: return '0';
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    case 8: return '8';
    case 9: return '9';
    case 10: return 'a';
    case 11: return 'b';
    case 12: return 'c';
    case 13: return 'd';
    case 14: return 'e';
    case 15: return 'f';
  }
}

void loop() {
  /*if((digitalRead(10) == 1)||(digitalRead(11) == 1)){
    StrJSON = StrJSON + "0;";
  }
  else
  {*/
  int ecg = (int)analogRead(A0);
  StrJSON = StrJSON + byteToAscii((unsigned char)((ecg >> 8) & 0x0F)) +byteToAscii((unsigned char)((ecg >> 4) & 0x0F)) + byteToAscii((unsigned char)(ecg & 0x0F));
  if(StrJSON.length() > 8066)
    StrJSON = "{\"val_senzor_ecg\":\"";
  //}
	int myBPM = pulseSensor.getBeatsPerMinute();      // Calculates BPM
  if(bluetooth.available()) {
    while(bluetooth.available()) bluetooth.read();
    //StrJSON[i++] = '\"';
    StrJSON = StrJSON + "\",\"val_senzor_puls\":\""+LastBPMRecorded + "\",\"val_senzor_umiditate\":\""+(float)dht.readHumidity()+"\",\"val_senzor_temperatura\":\""+(float)dht.readTemperature()+"\"}";
    for(auto &ch : StrJSON) {
      Serial.print(ch);
      bluetooth.print(ch);
    }
    LastBPMRecorded = 0;
    StrJSON = "{\"ecg\":\"";
  }
	if (pulseSensor.sawStartOfBeat()) {               // Constantly test to see if a beat happened
		//Serial.println("♥  A HeartBeat Happened ! "); // If true, print a message
		//Serial.print("BPM: ");
		//Serial.println(myBPM);                        // Print the BPM value
    LastBPMRecorded = myBPM;
	}
	delay(1000);
}