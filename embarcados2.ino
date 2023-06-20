
//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial

#include <CayenneMQTTESP32.h>
#include <Servo.h>
#include <stdio.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

#define POWER_SCALE_FACTOR 1000
// WiFi network info.
char ssid[] = "AndroidAP2B7B";
char wifiPassword[] = "xbzr6733";

static const int servoPin = 13;
const int BUFFER_SIZE = 100;
char buf[BUFFER_SIZE];

Servo servo;

//Insere máquina de estados.
enum State {
  AUTOMATIC,
  MANUAL
};

State currentState = AUTOMATIC;  // Set the initial state to Automatic

Adafruit_INA219 ina219;

//::::::::::::::::::::::::::::::::::::Variaveis globais:::::::::::::::::::::::::::::::::::::::::::
int ser; //serial
int aux;
static int ang; //angulo que vai pro servo
int value = 110;
int diff;

//LDRs
int analogValue; //ldr 0 (pino 34)
int analogValue1; //ldr 1 (pino 35)

//Posicao do mini Painel
int tolerancia = 30; 
int pos = 110;
const int minPos = 40;  // Minimum position limit
const int maxPos = 180;  // Maximum position limit

//int comando = 0; //Modo Manual

  float shuntvoltage = 0;
  float busvoltage_mV = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
//::::::::::::::::::::::::::::::::::Final das variáveis globais:::::::::::::::::::::::::::::::::::::::::::::

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "b8cfcda0-0a28-11ee-8485-5b7d3ef089d0";
char password[] = "74e1a29ea506ea597edbf2be9af5ffd726d26215";
char clientID[] = "c2b51140-0a28-11ee-8485-5b7d3ef089d0";


void setup() {
	Serial.begin(9600);
  
	Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  servo.attach(servoPin);
    if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  analogReadResolution(12);
  //ina219.setCalibration_32V_1A();
}

void loop() {
  Cayenne.loop();
  //Variaveis locais
      shuntvoltage = ina219.getShuntVoltage_mV();
      busvoltage_mV = ina219.getBusVoltage_V();
      current_mA = ina219.getCurrent_mA();
      power_mW = ina219.getPower_mW();
      loadvoltage = busvoltage_mV + (shuntvoltage / 1000);
      analogValue = 0.8 * analogRead(34);
      analogValue1 = analogRead(35);
      Serial.printf("%d, %d \n, %d \n", analogValue, analogValue1, pos);

  switch (currentState) {
    case AUTOMATIC:
      // 1. Adjust position based on LDR readings
      // 2. Decrease angle increment once it reaches 180 degrees
      // 3. Increase angle increment once it reaches 0 degrees
      // 4. Send sensor data to Cayenne
      diff = analogValue - analogValue1;
      //Ajusta posica de acordo com os LDRs
      if (abs(diff) > tolerancia) {
        if ((analogValue < analogValue1) && (pos < maxPos)) {
          pos+=ceil(diff/100);
        }
        if ((analogValue > analogValue1) && (pos > minPos)) {
          pos+=ceil(diff/100);
        } 
      }
      if(pos > maxPos) {pos = maxPos;} // reset the horizontal postion of the motor to 180 if it tries to move past this point
      if(pos < minPos) {pos = minPos;} // reset the horizontal position of the motor to 0 if it tries to move past this point
      servo.write(pos);

      // Decrease the angle increment once it reaches 180 degrees
      if (pos == maxPos) {
        servo.write(pos);
        delay(1000); // Optional delay for stability
        pos -= 1;
      }
      
      // Increase the angle increment once it reaches 0 degrees
      if (pos == minPos) {
        servo.write(pos);
        delay(1000); // Optional delay for stability
        pos += 1;
      }
    
      break;

    case MANUAL:
      // Manual mode logic
      // Handle commands received from Cayenne to control the servo position manually
      CAYENNE_IN_DEFAULT();
      pos = value;
      servo.write(pos);
      break;
  }
} 

// Default function for sending sensor data at intervals to Cayenne.
// You can also use functions for specific channels, e.g CAYENNE_OUT(1) for sending channel 1 data.
CAYENNE_OUT_DEFAULT()
{
	// Write data to Cayenne here. This example just sends the current uptime in milliseconds on virtual channel 0.
	Cayenne.virtualWrite(0, millis());
  Cayenne.virtualWrite(1, current_mA, TYPE_VOLTAGE, UNIT_MILLIVOLTS);
  Cayenne.virtualWrite(3, power_mW);
  Cayenne.virtualWrite(4, analogValue);
  Cayenne.virtualWrite(5, busvoltage_mV);
  Cayenne.virtualWrite(6, pos);
  Cayenne.virtualWrite(7, analogValue1);
  //Serial.println(current_mA);

	// Some examples of other functions you can use to send data.
	//Cayenne.celsiusWrite(1, 22.0);
	//Cayenne.luxWrite(2, 700);
	//Cayenne.virtualWrite(3, 50, TYPE_PROXIMITY, UNIT_CENTIMETER);
}

// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
	CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
  //value = getValue.asInt();
  if(request.channel == 4) {
    int nextState = getValue.asInt();
    if (nextState == 0) {
      currentState = MANUAL;
    } else {
      currentState = AUTOMATIC;
    }
/*    if (currentState == MANUAL) {
      currentState = AUTOMATIC;
    } else {
      currentState = MANUAL;
    }*/
  }
  else{
    value = getValue.asInt();
  }
  Serial.println(value);
	//Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}
