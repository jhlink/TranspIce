#include <SPI.h>
#include <SD.h>

// Project Four - Heat maker
//
#define LED 2 //connect LED to digital pin2
#define HEATER 3
#define THERM_1 A5   // In the middle of the cup
#define THERM_2 A3   // At the bottom of the cup
#define THERM_3 A2   // At the heatsink of the cup

#define THERMISTORNOMINAL 10000   // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25     // temp. for nominal resistance (almost always 25 C)
#define NUMSAMPLES 10              // Filter
#define BCOEFFICIENT 3950         // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 10000      // the value of the 'other' resistor
#define TOLERANCE .1

const int chipSelect = 4;
const float maxFreqVal = 80.0;

unsigned long startTime;
int volatileTemp = 0;
bool cont = true;
int count = 0;
int therm1Samp[NUMSAMPLES];
int therm2Samp[NUMSAMPLES];
int therm3Samp[NUMSAMPLES];
bool initialPreFreezeState = false;
float* focusedProbeTemp;
float* prevFocusedProbeTemp;

void setup()
{
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  pinMode(HEATER, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  // Start up the library
  // Setup SD Card for datalogging.
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // Start up the library
  analogReference(EXTERNAL);
}

void loop()
{
  float middleTherm_1 = 0.0;
  float bottomTherm_2 = 0.0;
  float heatSinkTherm_3 = 0.0;
  uint8_t i;
  float therm1Avg = 0.0;
  float therm2Avg = 0.0;
  float therm3Avg = 0.0;
  bool therm1Bool = false;
  bool therm2Bool = false;
  bool therm3Bool = false;
  String dataString = "";
  String tempStatus = "";

  //// Turn on LED
  digitalWrite(LED, HIGH);

  //// Thermistor 1 (middle of cup) and 2 (bottom of cup)

  // Filter out bad data. 
  // Sum N samples with delay

  do {
    for (i = 0; i < NUMSAMPLES; i++) {
      therm1Samp[i] = analogRead(THERM_1);
      therm2Samp[i] = analogRead(THERM_2);
      therm3Samp[i] = analogRead(THERM_3);
      delay(10);    
    }
    therm1Bool = tempWithinTol(therm1Samp);
    therm2Bool = tempWithinTol(therm2Samp);
    therm3Bool = tempWithinTol(therm3Samp);
  } while (therm1Bool || therm2Bool || therm3Bool);
  
   
  therm1Avg = avgArray(therm1Samp);
  therm2Avg = avgArray(therm2Samp);
  therm3Avg = avgArray(therm3Samp);

  // Convert value to thermistor resistance
  therm1Avg = valToThermRes(therm1Avg);
  therm2Avg = valToThermRes(therm2Avg);
  therm3Avg = valToThermRes(therm3Avg);
  
  // Convert average thermal resistance to temperature (F)
  middleTherm_1 = resToFahren(therm1Avg);
  bottomTherm_2 = resToFahren(therm2Avg);
  heatSinkTherm_3 = resToFahren(therm3Avg);


  ////
  ////  Heater logic

  // Beginning initialization state of the freezing process.
  // Cool container down to 33.0 F from variable intial temp.
  if (!initialPreFreezeState && bottomTherm_2 >= 33.0) {
    analogWrite(HEATER, 0);
  } else if (!initialPreFreezeState && bottomTherm_2 < 33.0) {
    initialPreFreezeState = true;
    startTime = millis();
    focusedProbeTemp = &bottomTherm_2;
    prevFocusedProbeTemp = &bottomTherm_2;
    volatileTemp = 70;  // Beginning at approx. 28%
  }

  if (initialPreFreezeState) {

    // Code to focus on which probe gain data from based on ice level.

    // Measuring in 30 minute intervals
    if (((millis() - startTime) % 1800000) == 0) {
      count++;
    }

    if ((*focusedProbeTemp >= 31.5) && (*focusedProbeTemp <= 32.5)) {
      if ((heatSinkTherm_3 >= middleTherm_1) && (middleTherm_1 > bottomTherm_2)) {
        // Do nothing. Probably use a go to statemnt or whatever...
        // Ideal spot to be in.
        tempStatus = "Ideal State";
      } else {
        tempStatus = "Stable";
      }
    } else if (*focusedProbeTemp > 32.5) {
      volatileTemp -= 3; // decreasing heat gen. power level by 1%
      tempStatus = "DecPow";
    } else if (*focusedProbeTemp < 31.5) {
      volatileTemp += 3; // increasing heat gen. power level by 1%
      tempStatus = "IncPow";
    }
  }

  if (volatileTemp > 100) {
    volatileTemp = 100;
  }

  analogWrite(HEATER, 70);

  //// SD Card datalogging
  // Create data string for heatSink, middleTherm_1, and bottomTherm_2
  dataString += String(millis());
  dataString += "\t";
  dataString += String(count);
  dataString += "\t";
  dataString += String(heatSinkTherm_3);
  dataString += "\t";
  dataString += String(middleTherm_1);
  dataString += "\t";
  dataString += String(bottomTherm_2);
  dataString += "\t";
  dataString += tempStatus;
  dataString += "\t";
  dataString += volatileTemp;

    // Open file and write to it.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("error opening datalog.txt");
  }
  delay(1000);
}

void printArray(int data[]) {
  for(int i = 0; i < NUMSAMPLES; i++) {
      Serial.print(String(resToFahren(valToThermRes(data[i]))));
      Serial.print(", ");
  }
  Serial.println();
}

bool tempWithinTol(int data[]) {
  // Average samples
  float dataAvg = resToFahren(valToThermRes(avgArray(data)));
  float stdDevVal = stdDev(data, dataAvg);
  Serial.print("Std dev: ");
  Serial.println(stdDevVal);

  if (stdDevVal > TOLERANCE) {
    return false;
  } else {
    return true;
  }
}

float stdDev(int data[], float mean) {
  float stdDevRes = 0.0;
  for (int i = 0; i < NUMSAMPLES; i++) {
    stdDevRes += sq(resToFahren(valToThermRes(data[i])) - mean);
  }
  stdDevRes /= NUMSAMPLES;
  stdDevRes = sqrt(stdDevRes);
  return stdDevRes;
}

float avgArray(int data[]) {
  // Average samples
  float dataAvg = 0;
  for (int i = 0; i < NUMSAMPLES; i++) {
    dataAvg += data[i];
  }
  dataAvg /= NUMSAMPLES;
  return dataAvg;
}

float valToThermRes(float value) {
  float result = SERIESRESISTOR / (1023 / value - 1);
  return result;
}

float resToFahren(float resistance) {
  float steinhart = 0.0;
  steinhart = resistance / THERMISTORNOMINAL;        // (R/Ro)
  steinhart = log(steinhart);                        // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);  // + (1/To)
  steinhart = 1.0 / steinhart;                       // Invert
  steinhart -= 273.15;                               // convert to C
  steinhart *= 9.0 / 5;                              // In Fahrenheight
  steinhart += 32;

  return steinhart;
}
