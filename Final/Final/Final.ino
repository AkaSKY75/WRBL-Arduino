#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math
#define DHTPIN 7
#define DHTTYPE DHT11

#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library
#include "SoftwareSerial.h";           // Used for Bluetooth Mate
#include <DHT.h>                     // DHT11

enum class TransmissionStates {
  READ_ECG,
  SHOULD_SEND,
  BUFFER_FULL
};

/*  JSON stuff  */
//unsigned char StrJSON[8066];
//String StrJSON;
char jsonBuffer[1001];
char str_temp_humidity[6];
char str_temp_temperature[6];
int ecg;
unsigned short jsonBufferIndex = 0;
TransmissionStates transmissionState = TransmissionStates::READ_ECG;
char command;
//unsigned short StrJSONIndex;

/*  DHT11  */
DHT dht(DHTPIN, DHTTYPE);
float LastReadHumidity = 0;
float LastReadTemperature = 0;


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

  cli();
  Serial.begin(9600);

  /* JSON */
  //memset(StrJSON, 0, 8066);
  //StrJSONIndex = 0;
  //strcpy(StrJSON, "{\"ecg\":\"")
  //StrJSON = "{\"val_senzor_ecg\":\"";
  memset(jsonBuffer, 0, 1000);
  jsonBufferIndex = 0;
  transmissionState = TransmissionStates::READ_ECG;
  command = -1;

  /* DHT */
  dht.begin();
  LastReadHumidity = 0;
  LastReadTemperature = 0;

  /* Bluetooth sensor config */
  bluetooth.begin(115200);
  /*bluetooth.print("$");
  bluetooth.print("$");
  bluetooth.print("$");
  delay(100);
  bluetooth.println("U,9600,N");
  bluetooth.begin(9600);*/

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

  /* INTERRUPTS */
  //set timer2 interrupt at 1000KHz
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; //initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 250; // = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 64 prescaler
  TCCR2B |= (1 << CS21) | (1 << CS20);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  sei();

}

void send_buffer() {
  //Serial.println("Buffer length:" +jsonBufferIndex);
  jsonBuffer[jsonBufferIndex] = '\n';
  for(jsonBufferIndex = 0; jsonBuffer[jsonBufferIndex] != '\n'; jsonBufferIndex++) {
    bluetooth.print(jsonBuffer[jsonBufferIndex]);
  }
  bluetooth.print('\n');
  bluetooth.flush();
  memset(jsonBuffer, 0, 1000);
  jsonBufferIndex = 0;
}

ISR(TIMER2_COMPA_vect) {
  //timer1 interrupt 1KHz (1ms)
  if (transmissionState == TransmissionStates::READ_ECG || transmissionState == TransmissionStates::SHOULD_SEND) {
    if (jsonBufferIndex != 1000) {
      ecg = (int)analogRead(A0);
      jsonBuffer[jsonBufferIndex++] = '0';
      jsonBuffer[jsonBufferIndex++] = byteToAscii((unsigned char)((ecg >> 8) & 0x0F));
      jsonBuffer[jsonBufferIndex++] = byteToAscii((unsigned char)((ecg >> 4) & 0x0F));
      jsonBuffer[jsonBufferIndex++] = byteToAscii((unsigned char)(ecg & 0x0F));
    } else if (transmissionState == TransmissionStates::SHOULD_SEND) {
      send_buffer();
      transmissionState = TransmissionStates::READ_ECG;
    } else {
      transmissionState = TransmissionStates::BUFFER_FULL;
    }
  }

  int myBPM = pulseSensor.getBeatsPerMinute();      // Calculates BPM

	if (pulseSensor.sawStartOfBeat()) {               // Constantly test to see if a beat happened
		//Serial.println("♥  A HeartBeat Happened ! "); // If true, print a message
		//Serial.print("BPM: ");
		//Serial.println(myBPM);                        // Print the BPM value
    LastBPMRecorded = myBPM;
  }
	
  LastReadHumidity = (float)dht.readHumidity();
  LastReadTemperature = (float)dht.readHumidity();

  if(bluetooth.available()) {
    command = (char)bluetooth.read();
    // Serial.println(command);
    if (command == '0') {
      if (transmissionState == TransmissionStates::BUFFER_FULL) {
        send_buffer();
      } else {
        transmissionState = TransmissionStates::SHOULD_SEND;
      }
    } else if (command == '1') {
      memset(jsonBuffer, 0, 1000);
      dtostrf(LastReadHumidity, 4, 2, str_temp_humidity);
      dtostrf(LastReadTemperature, 4, 2, str_temp_temperature);
      jsonBufferIndex = sprintf(jsonBuffer, "{\"val_senzor_puls\":\"%d\",\"val_senzor_umiditate\":\"%s\","
        "\"val_senzor_temperatura\":\"%s\"}", LastBPMRecorded, str_temp_humidity, str_temp_temperature
      );
      send_buffer();
      LastBPMRecorded = 0;
      transmissionState = TransmissionStates::READ_ECG;
    }
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
    //delay(1);
}