// Project Four - Noise maker
//
#define LED 2 //connect LED to digital pin2
//#define VIBRATOR_2 6
#define VIBRATOR_1 5
unsigned long time;
unsigned long tenMin = 600000;
unsigned long fiveMin = 300000;

void setup()
{
//  pinMode(VIBRATOR_2, OUTPUT);
  pinMode(VIBRATOR_1, OUTPUT);
  pinMode(LED, OUTPUT);     
}

void loop()
{
  analogWrite(VIBRATOR_1, 255); 
  digitalWrite(LED, HIGH);   // set the LED on

  
//  time = millis();
//  if ((time % fiveMin) == 0) {
//  analogWrite(VIBRATOR_2, 127);
//    delay(1000 * 30);
//    analogWrite(VIBRATOR, 0);
//  }
//  delay(1000);
}
   
