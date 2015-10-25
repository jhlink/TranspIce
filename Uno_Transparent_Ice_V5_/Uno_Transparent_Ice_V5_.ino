#include <SPI.h>
#include <SD.h>
#include <Wire.h>

const int chipSelect = 4;
char command[150]; // expect 7 char + 1 end byte
bool beginProg = true;
void setup()
{
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
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

  // Start the I2C Bus as Slave on address 9
  Wire.begin(9);

  // Attach a function to trigger when something is received.
  Wire.onReceive(receiveEvent);
}

void loop()
{
  // Open file and write to it.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  if (dataFile) {
    if (beginProg) { 
      dataFile.println("Start of new data collection\n");
      beginProg = false; 
    }
    
    dataFile.println(String(command));
    dataFile.close();
    Serial.println(String(command));
  } else {
    Serial.println("error opening datalog.txt");
  }
  delay(1000);
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void receiveEvent(int howMany)
{
  int receiveByte = 0; // set index to 0
  while(Wire.available()) // loop through all incoming bytes
  {
    command[receiveByte] = Wire.read(); // receive byte as a character
    receiveByte++; // increase index by 1
  }
}

