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

uint8_t broadcastAddress[] = {0x24, 0x62, 0xAB, 0xF3, 0xAF, 0xD0};

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
  Serial.println("WiFi connected");
 
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 34;
  config.pin_d1 = 13;
  config.pin_d2 = 14;
  config.pin_d3 = 35;
  config.pin_d4 = 39;
  config.pin_d5 = 38;
  config.pin_d6 = 37;
  config.pin_d7 = 36;
  config.pin_xclk = 4;
  config.pin_pclk = 25;
  config.pin_vsync = 5;
  config.pin_href = 27;
  config.pin_sscb_sda = 18;
  config.pin_sscb_scl = 23;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);

 // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 
  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
 
  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  read_face_id_from_flash(&id_list);// Read current face data from on-board flash


    ///// Init ESP-NOW////////////////////////////////////////////////////////
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
void rzoCheckForFace() {
  
  if (run_face_recognition()) { // face recognition function has returned true
    Serial.println("Face recognised");
    recognize=true;
  }
  else{
    Serial.println("Face NOT recognised");
    recognize=false;}
 
}
 
bool run_face_recognition() {
  bool faceRecognised = false; // default
  int64_t start_time = esp_timer_get_time();
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
 
  int64_t fb_get_time = esp_timer_get_time();
  Serial.printf("Get one frame in %u ms.\n", (fb_get_time - start_time) / 1000); // this line can be commented out
 
  image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  uint32_t res = fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  if (!res) {
    Serial.println("to rgb888 failed");
    dl_matrix3du_free(image_matrix);
  }
 
  esp_camera_fb_return(fb);
 
  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);
 
  if (net_boxes) {
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK) {
 
      int matched_id = recognize_face(&id_list, aligned_face);
      if (matched_id >= 0) {
        Serial.printf("Match Face ID: %u\n", matched_id);
        faceRecognised = true; // function will now return true
      } else {
        Serial.println("No Match Found");
        matched_id = -1;
      }
    } else {
      Serial.println("Face Not Aligned");
    }
 
    dl_lib_free(net_boxes->box);
    dl_lib_free(net_boxes->landmark);
    dl_lib_free(net_boxes);
  }
 
  dl_matrix3du_free(image_matrix);
  return faceRecognised;
}
 
 
void loop() {
  if(activar){
  for (int i=0;i<10 || !recognize;i++){
  rzoCheckForFace();
  delay(250);
  }
  reconocimiento.message=recognize;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &reconocimiento, sizeof(reconocimiento));
  
  activar=false;//una vez reconocida o no una cara se apague la camara hasta que se vuelva a detectar un lector NFC
  }
  else{
     Serial.println("La camara esta apagada");
    }
    delay(1000);
}