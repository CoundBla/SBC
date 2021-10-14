#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

//FIRMWARE
#define FIRMWARE_VERSION 0.01

//Datos Wifi
const char* host = "esp32";
const char* ssid = "Te Espio";
const char* password = "123789qwerty";


//Tiempo dormido
#define DEEP_SLEEP_TIME 30


//Pines
#define LED 27

unsigned long t1 = 0;
unsigned long t2 = 0;

//Función que se utilizará para asignar valores iniciales a las variables globales. Se incluyen los pinMode(...)
void SetGlobalVariables(){
  pinMode(LED,OUTPUT);  
  t1 = millis();
}
//Función que se utiliza para conectar al WIFI.
void ConnectToWifi(){
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //El serial.println sirve en este caso???
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
//Función que parpadea un led conectado al pin LED
void ParpadearLuz(){
      t2= millis();
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);    
  
    if(t2 >= (t1+10000)){
      //goToDeepSleep(DEEP_SLEEP_TIME*1000000);
    }
  }
void UpdateFirmWare(){
  //Actualizamos el firmware
}

void CheckFirmwareVersion(){
  //Recuperamos la última versión de firmware de Firebase
  //Si la versión de firebase es mayor que la actual, actualizamos el código
    UpdateFirmWare();
}

//Función que duerme durante un tiempo en milisegundos. Tras ese tiempo se despierta, y se ejecuta el setup.
void goToDeepSleep(long timeInMicroSeconds){
    Serial.println("Procesador a dormir ...");

  esp_sleep_enable_timer_wakeup(timeInMicroSeconds);
  esp_deep_sleep_start();
  
}

void setup(){
  Serial.begin(9600);

  ConnectToWifi();
  SetGlobalVariables();
  //CheckFirmwareVersion();
  
  //Serial.println("enviando.....");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() == WL_CONNECTED){
    ParpadearLuz();
  }
  if(WiFi.status() == WL_DISCONNECTED){
    ConnectToWifi();
  }
}