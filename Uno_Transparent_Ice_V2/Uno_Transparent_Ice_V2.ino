#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Project Four - Heat maker
//
#define LED 2 //connect LED to digital pin2
#define HEATER 3
#define ONE_WIRE_BUS 7

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

float ctemp = 0.0;
unsigned long startTime = millis();
float maxFreqVal = 80.0;
int volatileTemp = 40;
bool cont = true;
int count = 0;

void setup()
{
  pinMode(HEATER, OUTPUT);
  pinMode(LED, OUTPUT);     
  Serial.begin(9600);

  // Start up the library
  sensors.begin();
  Wire.begin(); 
}

void loop()
{
  digitalWrite(LED, HIGH);   // set the LED on

 
  sensors.requestTemperatures(); // Send the command to get temperatures
//  Serial.print("Temperature for the device 1 (index 0) is: ");

  ctemp = DallasTemperature::toFahrenheit(sensors.getTempCByIndex(0));
  Serial.println(ctemp);  
  
  if (ctemp > 33.0) {
    analogWrite(HEATER, 0);
  } else if (ctemp <= 33.0 && ctemp >= 31.0) {
    if (((millis() - startTime) % 60000) == 0) {
      count++;
      if ((volatileTemp >= 5) && (count >= 120)) {
        volatileTemp = volatileTemp - 5;
        count = 0;
        analogWrite(HEATER, volatileTemp);
      }

      if (volatileTemp <= 10) {
        cont = false;
      }
    }
  } else if (ctemp <= 29.0 && cont) {
    analogWrite(HEATER, maxFreqVal);
  }
  
  delay(1000);
}
   
