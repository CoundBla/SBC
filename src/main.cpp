#include <Arduino.h>

#define DEEP_SLEEP_TIME 30
#define LED 2

unsigned long t1 = 0;
unsigned long t2 = 0;

void goToDeepSleep(long timeInMicroSeconds){
    Serial.println("Procesador a dormir ...");

  esp_sleep_enable_timer_wakeup(timeInMicroSeconds);
  esp_deep_sleep_start();
  
}

void setup(){
  Serial.begin(9600);

  //Serial.println("midiendo co2 en el aula con el sension....");

  //Serial.println("enviando.....");
  pinMode(LED,OUTPUT);  
  t1 = millis();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  t2= millis();
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);    
  
  if(t2 >= (t1+10000)){
    goToDeepSleep(DEEP_SLEEP_TIME*1000000);
  }
                    
}