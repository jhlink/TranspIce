#include <OneWire.h>
#include <DallasTemperature.h>



// which analog pin to connect
#define THERMISTORPIN A2  
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    



 #define ONE_WIRE_BUS 7
 // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

 
int samples[NUMSAMPLES];
 
void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  analogReference(EXTERNAL);
}
 
void loop(void) {
  uint8_t i;
  float average;
  float ctemp;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  Serial.print("Average analog reading "); 
  Serial.println(average);
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance "); 
  Serial.println(average);
 
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  // In Fahrenheight

  steinhart *= 9.0/5;
  steinhart += 32;
 
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" *C");

  sensors.requestTemperatures(); // Send the command to get temperatures
//  Serial.print("Temperature for the device 1 (index 0) is: ");

  ctemp = DallasTemperature::toFahrenheit(sensors.getTempCByIndex(0));
  Serial.print("This is oneWire Temp: ");
  Serial.println(ctemp);
 
  delay(1000);
}
