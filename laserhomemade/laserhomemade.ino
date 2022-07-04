/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-light-sensor
 */
#include <Servo.h>


Servo myservo;
int pos = 0; 


const int pinLaser = 2; 
void setup() {
  // initialize serial communication at 9600 bits per second:
 pinMode(pinLaser, OUTPUT);
  digitalWrite(pinLaser, HIGH);
  myservo.attach(9); 
  Serial.begin(9600);
}

void loop() {
  // reads the input on analog pin A0 (value between 0 and 1023)
  int analogValue = analogRead(A0);

  Serial.print("Analog reading = ");
  Serial.print(analogValue);   // the raw analog reading

  // We'll have a few threshholds, qualitatively determined
 if (analogValue < 1000) {
    Serial.println(" 0");
    pos=0;
    myservo.write(pos); 
  } else {
    Serial.println(" 1");
    pos=180;
    myservo.write(pos); 
  }

  delay(500);
}
