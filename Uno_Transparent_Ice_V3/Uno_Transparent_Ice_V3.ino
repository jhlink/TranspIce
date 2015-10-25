#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>

// Project Four - Heat maker
//
#define LED 2 //connect LED to digital pin2
#define HEATER 3
#define ONE_WIRE_BUS 7
#define THERM_1 A0   // In the middle of the cup
#define THERM_2 A1   // At the bottom of the cup

#define THERMISTORNOMINAL 10000   // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25     // temp. for nominal resistance (almost always 25 C)
#define NUMSAMPLES 5              // Filter
#define BCOEFFICIENT 3950         // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 10000      // the value of the 'other' resistor

const int chipSelect = 4;
const float maxFreqVal = 80.0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

unsigned long startTime;
int volatileTemp = 40;
bool cont = true;
int count = 0;
int therm1Samp[NUMSAMPLES];
int therm2Samp[NUMSAMPLES];
bool initialPreFreezeState = false;
float * focusedProbeTemp = 0.0;
float * prevFocusedProbeTemp = 0.0;

void setup()
{
  pinMode(HEATER, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  // Start up the library
  sensors.begin();

  // Setup SD Card for datalogging.
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

void loop()
{
  float heatSinkTemp = 0.0;
  float therm1 = 0.0;
  float therm2 = 0.0;
  uint8_t i;
  float therm1Avg = 0;
  float therm2Avg = 0;
  String dataString = "";
  String tempStatus = "";

  //// Turn on LED
  digitalWrite(LED, HIGH);

  //// Request for temperature(F) to probe closest to heat sink.
  sensors.requestTemperatures();
  heatSinkTemp = DallasTemperature::toFahrenheit(sensors.getTempCByIndex(0));

  //// Thermistor 1 (middle of cup) and 2 (bottom of cup)

  // Sum N samples with delay
  for(i = 0; i < NUMSAMPLES; i++) {
    therm1Samp[i] = analogRead(THERM_1);
    therm2Samp[i] = analogRead(THERM_2);
    delay(10);
  }

  // Average samples
  for (i = 0; i < NUMSAMPLES; i++) {
     therm1Avg += therm1Samp[i];
     therm2Avg += therm2Samp[i];
  }
  therm1Avg /= NUMSAMPLES;
  therm2Avg /= NUMSAMPLES;

  // Convert value to thermistor resistance
  therm1Avg = valToThermRes(therm1Avg);
  therm2Avg = valToThermRes(therm2Avg);

  // Convert average thermal resistance to temperature (F)
  therm1 = resToFahren(therm1Avg);
  therm2 = resToFahren(therm2Avg);

  ////  Heater logic

  // Beginning initialization state of the freezing process.
  // Cool container down to 33.0 F from variable intial temp.
  if (!initialPreFreezeState && therm2 >= 33.0) {
    volatileTemp = 0;
  } else if (!initialPreFreezeState && therm2 < 33.0) {
    initialPreFreezeState = true;
    startTime = millis();
    focusedProbeTemp = &therm2;
    prevFocusedProbeTemp = &therm2;
    volatileTemp = 70/255;  // Beginning at approx. 28%
  }

  if (initialPreFreezeState) {

    // Measuring in 30 minute intervals
    if (((millis() - startTime) % 1800000) == 0) {
      // One count = one 30 minute chunk of time
      count++;
    }

    // Code to shift focus on which probe to target based on ice level.
    if (*prevFocusedProbeTemp < 30.5) {
      if (count >= 2 && focusedProbeTemp == &therm2) {
        focusedProbeTemp = &therm1;
      } else if (count >= 4 && focusedProbeTemp == &therm1) {
        prevFocusedProbeTemp = &therm1;
        focusedProbeTemp = &heatSinkTemp;
      }
    }

    if (*focusedProbeTemp >= 31.5 && (*focusedProbeTemp <= 32.5) {
      if ((heatSinkTemp > therm1) && (therm1 > therm2)) {
        // Do nothing. Probably use a go to statemnt or whatever...
        // Ideal spot to be in.
        tempStatus = "Stable";
      }
    } else if ((*focusedProbeTemp > 32.5) {
      volatileTemp -= 3; // decreasing heat gen. power level by 1%
      tempStatus = "DecPow";
    } else if ((*focusedProbeTemp < 31.5) {
      volatileTemp += 3; // increasing heat gen. power level by 1%
      tempStatus = "IncPow";
    }
  }

  analogWrite(HEATER, volatileTemp);


  // if (ctemp > 33.0) {
  // } else if (ctemp <= 33.0 && ctemp >= 31.0) {
  //   if (((millis() - startTime) % 60000) == 0) {
  //     count++;
  //     if ((volatileTemp >= 5) && (count >= 120)) {
  //       volatileTemp = volatileTemp - 5;
  //       count = 0;
  //       analogWrite(HEATER, volatileTemp);
  //     }
  //
  //     if (volatileTemp <= 10) {
  //       cont = false;
  //     }
  //   }
  // } else if (ctemp <= 29.0 && cont) {
  //   analogWrite(HEATER, maxFreqVal);
  // }

  //// SD Card datalogging

  // Create data string for heatSink, therm1, and therm2
  dataString += String(millis());
  dataString += "\t";
  dataString += String(heatSinkTemp);
  dataString += "\t";
  dataString += String(therm1);
  dataString += "\t";
  dataString += String(therm2);
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

float valToThermRes(float value) {
  float result = SERIESRESISTOR / (1023 / value - 1);
  return result;
}

float resToFahren(float resistance) {
  float steinhart = 0.0;
  steinhart = average / THERMISTORNOMINAL;           // (R/Ro)
  steinhart = log(steinhart);                        // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);  // + (1/To)
  steinhart = 1.0 / steinhart;                       // Invert
  steinhart -= 273.15;                               // convert to C
  steinhart *= 9.0/5;                                // In Fahrenheight
  steinhart += 32;

  return steinhart;
}
