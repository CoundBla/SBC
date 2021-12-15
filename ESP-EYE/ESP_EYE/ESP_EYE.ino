#include "esp_camera.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "fr_flash.h"
#include <esp_now.h>

#include <WiFi.h>

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7


const char* ssid = "Te Espio";
const char* password = "123789qwerty";


///ESP NOW /////////////////////////////////////////////////////////
uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0xC7, 0x17, 0x98};

// Definir variables para enviar
bool recognize;

// variables que recibe
bool activar;

String success;

//Estructura para enviar datos
// Debe coincidir con la estructura del receptor
typedef struct struct_message {
    bool message;
} struct_message;


struct_message reconocimiento;

struct_message activacion;

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
  memcpy(&activacion, incomingData, sizeof(activacion));
  activar=activacion.message;
  Serial.print("activar de camara: ");
  Serial.println(activar);
}



///////////////ESP EYE///////////////////////////////////////////////////////
static inline mtmn_config_t app_mtmn_config()
{
  mtmn_config_t mtmn_config = {0};
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
  return mtmn_config;
}
mtmn_config_t mtmn_config = app_mtmn_config();
 
static face_id_list id_list = {0};
dl_matrix3du_t *image_matrix =  NULL;
camera_fb_t * fb = NULL;
 
dl_matrix3du_t *aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
 
void setup() {
  Serial.begin(115200);
  
WiFi.mode(WIFI_AP_STA);

 WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
... (147 líneas restantes)
