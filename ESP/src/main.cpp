#include <WiFi.h>
#include <HttpsOTAUpdate.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Firebase_ESP_Client.h>
#include <Firebase.h>
#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>
#include <LiquidCrystal.h>

//FIRMWARE
#define CheckFirmwareFL 0   
#define FIRMWARE_VERSION 0.1

static const char *server_certificate = "-----BEGIN CERTIFICATE-----\nMIIC/DCCAeSgAwIBAgIIKn4P6HR6+bowDQYJKoZIhvcNAQEFBQAwIDEeMBwGA1UE\nAxMVMTExNDQ1MjE2MzY4OTk2MDA5MDkwMCAXDTIxMTExMTE2NTYzOFoYDzk5OTkx\nMjMxMjM1OTU5WjAgMR4wHAYDVQQDExUxMTE0NDUyMTYzNjg5OTYwMDkwOTAwggEi\nMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCejuG4QwmltwGgvUW8qlQImXpH\n2UtwYtixGtShto5knYpu3cp8u0FTzlWkiTZMAVH7QoVNjradBEZK706GBxtsEqSr\nwMdCLMm0g9GwrmGRzHGsG8c2vViwSAb79E87D/ZTweuGaWaUFTb3GileKv2OKs8o\n+Q58v0ND4Qti5vR6Cjn49gdUYSNY7R7QKm5u+1iipiHXICAFsFyzRz3EaywYSpAn\nipTbD2XduWoRqMNOz3UebSjZQBOfoK1dnbXbCpTBRpiYz/RSfQNn5DHfiHPd9977\nD+93I8d1geVy/6grbgPXp9gtSDAq7OQcFFQMztT22hR9uqU9lgOTsTWBMxtzAgMB\nAAGjODA2MAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQDAgeAMBYGA1UdJQEB/wQM\nMAoGCCsGAQUFBwMCMA0GCSqGSIb3DQEBBQUAA4IBAQAK2IJe8XkzBjy30qlL+UlJ\n2nnUfaOBb7ZjkYvikjbJYnUwzMKnn59XFadkIOW9gyLhwAgGyjy2skX3rE59DUkt\nYRfVEXUxa2mo+Cnx/SfMRT4wDT5r1QGnIGhk+84LHcPHb4G9T3k/LqF0RAEOciDG\nGo8feq2SLESUvrea0pJfytmQSzj2KIlFRhWQCF0rku6cDW6/oQq/e2rXRC74fLWP\nfkZk0kwO/ptTbty7DwymFX3wXPMsgILQZ78wYD5Zn8hMEdkTywaHHNkT3j4ECJzs\n6FNMm7e81AIkgQ6KIGfA55s38XUlWpTCx8RRk+BJpH2WESjJN7JMO5ihqCUhMg03\n-----END CERTIFICATE-----\n";

static HttpsOTAStatus_t otastatus;

//Datos Wifi
const char* host = "esp32";
const char* ssid = "Te Espio";
const char* password = "123789qwerty";
//const char* ssid = "DIGIFIBRA-GheA-HOME";
//const char* password = "123789qwerty";
//Datos ESP32
#define fireBase_Email "door1@sbcsmartdoor.com"
#define fireBase_Pwd "123789"
#define fireBase_ID "qpz3sCm9dJhaH4WvL0jZEi7Ej7J3"
#define fireBase_Code "SD-0001"
#define fireBase_API_KEY "AIzaSyC1_5OpxQTw2T_b8_gSfOJdw0VIgB5wmQo"
#define fireBase_AuthDomain "sbc-esp32-smartdoor.firebaseapp.com"
//URL API
String serverName = "https://europe-west1-sbc-esp32-smartdoor.cloudfunctions.net/";
String lastFirmwareUrl = "firmware/GetLastFirmWare";
String DoorNFCUrl = "door/IsValidCode";
//Variables para HTTPS
String httpRequest;
double firmWareVersionNum;
String firmWareVersionURL;

//Tiempo dormido
#define DEEP_SLEEP_TIME 30

//Pines
#define LED 27
const int pinRST = 15;  // Pin RST del módulo RC522
const int pinSDA = 5; // pin SDA del módulo RC522
const int buzzer = 27;
const int led = 2;//Led onboard del esp32

//Variables de tiempo
unsigned long t1 = 0;
unsigned long t2 = 0;

unsigned long lastTimeUpdateFW = 0;
unsigned long FirmWareUpdateTimer = 30000; //millisecons. 60k ms = 1 min.

//RFID
#define CheckRFIDFL 1 
MFRC522 rfid(pinSDA, pinRST);
String urlCaracteres;

//ESP EYE
uint8_t broadcastAddress[] = {0xC4, 0x4F, 0x33, 0x18, 0xAE, 0x2D};
//MAC ESP1: 24:62:AB:F3:AF:D0
// Definir variables para enviar
bool activar;
// variables que recibe
bool recognize;
String success;
//Estructura para enviar datos
// Debe coincidir con la estructura del receptor
typedef struct struct_message {
    bool message;
} struct_message;

struct_message reconocimiento;
struct_message activacion;

//DISPLAY
// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;
const int RS = 13, EN = 14, d4 = 27, d5 = 26,d6 = 25 , d7 = 33;
LiquidCrystal lcd(RS,EN,d4, d5, d6, d7);


String idOK = "Identificacion correcta";
String idErr = "Identificacion incorrecta";
int validoNFC = 1;
int validoFace=1;

int encendido=0;
//Declaramos funcionces
void setFirmwareInfo(JSONVar obj);
void SetGlobalVariables();
void ConnectToWifi();
void ParpadearLuz();
void bip(int demora);
void HttpEvent(HttpEvent_t *event);
void UpdateFirmWare();
String httpGETRequest(String serverName);
String httpPOSTRequest(String serverName, String data);
void CheckFirmwareVersion();
void setFirmwareInfo(JSONVar obj);
void CheckRFID();
int isValidRFID(String nfcID);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void scrollText(int row, String message, int delayTime, int lcdColumns);
void DisplayMessage(String msg);
void goToDeepSleep(long timeInMicroSeconds);

//Función que se utilizará para asignar valores iniciales a las variables globales. Se incluyen los pinMode(...)
void SetGlobalVariables(){
  pinMode(LED,OUTPUT);  
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);
  t1 = millis();
  //RFID
  SPI.begin();
  rfid.PCD_Init();//Inicilializar lector

  //ESP-EYE
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
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

//Sonido del buzzer
void bip(int demora)
{
  digitalWrite(buzzer, HIGH);
  delay(demora);
  digitalWrite(buzzer, LOW);  
}


void HttpEvent(HttpEvent_t *event)
{
    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
            Serial.println("Http Event Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            Serial.println("Http Event On Connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            Serial.println("Http Event Header Sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_ON_FINISH:
            Serial.println("Http Event On Finish");
            break;
        case HTTP_EVENT_DISCONNECTED:
            Serial.println("Http Event Disconnected");
            break;
    }
}


void UpdateFirmWare(){
  //Actualizamos el firmware OTA
  
  HttpsOTA.onHttpEvent(HttpEvent);
    Serial.println("Starting OTA");
    //int i = firmWareVersionURL.length();
    const char *url;
    url = firmWareVersionURL.c_str();
    uint32_t free_heap_size=0, min_free_heap_size=0;
    free_heap_size = esp_get_free_heap_size();
    min_free_heap_size = esp_get_minimum_free_heap_size(); 
    printf("\n free heap size = %d \t  min_free_heap_size = %d \n",free_heap_size,min_free_heap_size);
    Serial.println(url);
    HttpsOTA.begin(url, server_certificate); 
}

//Se realiza un get, y la función devuelve el string correspondiente
String httpGETRequest(String serverName) {
  HTTPClient http;

  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

//Se realiza un post, y la función devuelve el string correspondiente
//data debe ser un String, este string será un json con los datos necesarios para la peticion
String httpPOSTRequest(String serverName, String data) {
  HTTPClient http;

  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);

  http.addHeader("Content-Type", "application/json");
  Serial.println(serverName);
  Serial.println(data);
  // Send HTTP POST request
  int httpResponseCode = http.POST(data);
  String payload = "{}"; 
  

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}




void CheckFirmwareVersion(){
  //Recuperamos la última versión de firmware de Firebase
  //Parte del código sacado de /* https://randomnerdtutorials.com/esp32-http-get-post-arduino/ */
  //Si la versión de firebase es mayor que la actual, actualizamos el código
    HTTPClient http;

    String urlCheckUpdate = serverName + lastFirmwareUrl;

    httpRequest = httpGETRequest(urlCheckUpdate);

    JSONVar queryObjs = JSON.parse(httpRequest);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(queryObjs) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }
  
    Serial.print("JSON object = ");
    Serial.println(queryObjs);
  
    // myObject.keys() can be used to get an array of all the keys in the object
    JSONVar keys = queryObjs.keys();
  
    for (int i = 0; i < keys.length(); i++) {
      JSONVar value = queryObjs[keys[i]];
      Serial.print(keys[i]);
      Serial.print(" = ");
      Serial.println(value);
      
      setFirmwareInfo(value);
      if(firmWareVersionNum > FIRMWARE_VERSION){
          Serial.println("ACTUALIZANDO OTA...");
          UpdateFirmWare();
          //Ejecutar OTA
      }
    }
    Serial.print("1 = ");
    Serial.println(firmWareVersionNum);
    Serial.print("2 = ");
    Serial.println(firmWareVersionURL);
}

void setFirmwareInfo(JSONVar obj){
  JSONVar info = obj.keys();
  for(int i = 0; i < info.length(); i++){
    JSONVar key = info[i];
    JSONVar value = obj[info[i]];
    String keyStr;
    keyStr = key;
    Serial.print(info[i]);
    Serial.print(" = ");
    Serial.println(value);
    Serial.println(keyStr);
    if(keyStr == "Version"){
      firmWareVersionNum = value;
    }
    else{
      firmWareVersionURL = value;
    }
  }
}

void CheckRFID(){
  if (rfid.PICC_IsNewCardPresent())  // Hay una nueva tarjeta presente
  {
    if (rfid.PICC_ReadCardSerial())  // Leemos el contenido de la tarjeta
    {
      Serial.println("UID de la tarjeta:");
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        urlCaracteres += rfid.uid.uidByte[i];
      }
      Serial.print(urlCaracteres);
      Serial.println();
      
      if(urlCaracteres != NULL || urlCaracteres != "")
      {//Estas son las dos tarjeta registradas
        //urlCaracteres=="2281224977" || urlCaracteres=="19712918253"
        if(WiFi.status()== WL_CONNECTED){ // Comprueba que la wifi todavía esté conectada
          if(isValidRFID(urlCaracteres)==1){
            //RECONOCIMIENTO FACIAL
            activacion.message=true;
            esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &activacion, sizeof(activacion));
            for(int i = 0; i<20; i++){
              Serial.println("Reconociendo Cara");
              delay(1000);
            }
            if(reconocimiento.message){
               //TODO: Revisar con diego
            Serial.println("Tarjeta Correcta");
            DisplayMessage(idOK);
            //TODO: Revisar los leds
            digitalWrite(led, HIGH);
            //Se escribe registro en base de datos.
            //SetNewRegister(urlCaracteres);
            //Se envian registros a ThingsBoard
            }
            else{
              Serial.println("acceso denegado");
              DisplayMessage("FUERA");
            }
            
          }
          else{
            Serial.println("Acceso denegado");
            //bip(750);
            DisplayMessage("Acceso denegado");
          }
          Serial.print("CONECTADO");
        }
        else 
        {
          Serial.println("WiFi desconectado");
          DisplayMessage("WiFi desconectado");
        }
      }
      else
      {
        Serial.println("Tarjeta no válida");
        //bip(750);
        DisplayMessage("Tarjeta no válida");
        delay(1250);
      }
    }
  }
  urlCaracteres=""; 
}

int isValidRFID(String nfcID){
  int isvalid = 0;

  HTTPClient http;

  String urlCheckUpdate = serverName + DoorNFCUrl;
  String data = "{\"id\":\"";
  data = data + fireBase_ID;  
  data = data + "\",\"nfcID\":\"";
  data = data + nfcID;
  data = data + "\"}";
  /*strcat(data,"{\"id\":\"");
  strcat(data,fireBase_ID);
  strcat(data,",\"nfcID\":\"");
  strcat(data,nfcID);
  strcat(data,"\"}");
*/
  httpRequest = httpPOSTRequest(urlCheckUpdate,data);

  JSONVar queryObjs = JSON.parse(httpRequest);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(queryObjs) == "undefined") {
    Serial.println("Parsing input failed!");
    return 0;
  }

  Serial.print("JSON object = ");
  Serial.println(queryObjs);

  // myObject.keys() can be used to get an array of all the keys in the object
  JSONVar keys = queryObjs.keys();

  for (int i = 0; i < keys.length(); i++) {
    JSONVar value = queryObjs[keys[i]];
    String keystr;
    Serial.print(keys[i]);
    Serial.print(" = ");
    Serial.println(value);
    keystr = keys[i];
    if(keystr == "isValid"){
      isvalid = value;
    }    
    //setFirmwareInfo(value);

  }
  return isvalid;
}

//Comunicacion con ESP EYE
// Devolución de llamada cuando se envían datos
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}


// Devolución de llamada cuando se reciben datos
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&reconocimiento, incomingData, sizeof(reconocimiento));
  recognize=reconocimiento.message;
  Serial.print("Se ha reconocido la cara: ");
  Serial.println(recognize);
}

//DISPLAY
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void DisplayMessage(String msg){
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print scrolling message
  lcd.display();
  scrollText(1, msg, 250, lcdColumns);
  lcd.clear();
  delay(5);  
  lcd.noDisplay();
}
//Función que duerme durante un tiempo en milisegundos. Tras ese tiempo se despierta, y se ejecuta el setup.
void goToDeepSleep(long timeInMicroSeconds){
    Serial.println("Procesador a dormir ...");

  esp_sleep_enable_timer_wakeup(timeInMicroSeconds);
  esp_deep_sleep_start();
  
}

void setup(){
  Serial.begin(115200);

  ConnectToWifi();
  SetGlobalVariables();    
  //Serial.println("enviando.....");
    
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() == WL_CONNECTED){
    ParpadearLuz();
    if(CheckFirmwareFL == 1)
    {
      if((millis() - lastTimeUpdateFW)> FirmWareUpdateTimer){
        CheckFirmwareVersion();
        lastTimeUpdateFW = millis();
      }
    }

    //Comprobación de RFID
    if(CheckRFIDFL == 1)
    {
      CheckRFID();
    }

    /*if(isValidRFID("2281224977") == 1){
      Serial.println("Tarjeta Correcta");
    }
    else{
      Serial.println("Tarjeta incorrecta");
    }*/
    delay(1000);
  }
  if(WiFi.status() == WL_DISCONNECTED){
    ConnectToWifi();
  }
}