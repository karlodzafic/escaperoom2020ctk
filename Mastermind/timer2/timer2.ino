
#include <LiquidCrystal.h>
//LCD DISPLAY
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int inputsignal=4;

void setup() {
   lcd.begin(16, 2);
  pinMode(inputsignal, INPUT);

}

void loop() {

    lcd.setCursor(2, 2);
  for(int minute=60;minute>=0;minute=minute-1){
    for(int sekunde=60;sekunde>=0;sekunde=sekunde-1){
      lcd.print(minute);
      lcd.print(":");
      lcd.print(sekunde);
      delay(1000);
       lcd.clear();
      if(digitalRead(inputsignal)==HIGH){
        lcd.print("GOTOVO");
        exit(1);
        delay(5000);
        }
      }
    }

  
 }
