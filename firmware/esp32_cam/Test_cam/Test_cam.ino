
#include <esp_now.h>
#include "esp_camera.h"
#include <WiFi.h>

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"
#include "base64.h"
#define BLYNK_TEMPLATE_ID "TMPL6mus-ARyx"
#define BLYNK_TEMPLATE_NAME "SmartDoorN2"
#define BLYNK_AUTH_TOKEN "AFoEJrt_jCD4qfCTVUG-IP57tfOLLU4E"
const char* ssid = "H Tiến"; //WiFi SSID
const char* password = "10102003"; //WiFi Password
#include <Blynk.h>
#include<BlynkSimpleEsp32.h>
void startCameraServer();

boolean matchFace = false;

long prevMillis=0;
int interval = 6000;  //DELAY
uint8_t esp8266Address[] = {0x8C, 0xAA, 0xB5, 0xF4, 0x2F, 0x6C};
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Trang thai gui: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Thanh cong" : "That bai");
}

void sendUnlockSignal() {
  const char *msg = "FACE_RECOGNIZED";
  esp_err_t result;
  int attempts = 0;
  const int maxAttempts = 3;

  while (attempts < maxAttempts) {
    result = esp_now_send(esp8266Address, (uint8_t *)msg, strlen(msg));
    if (result == ESP_OK) {
        Serial.println("Sent unlock signal");
        return; // Thoát ngay khi gửi thành công
    } else {
        Serial.println("Error sending unlock signal, retrying...");
        attempts++;
        delay(1000); // Đợi một chút trước khi thử lại
    }
  }
  
  Serial.println("Failed to send unlock signal after multiple attempts");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Lỗi khởi tạo ESPNOW");
    return;
  }
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, esp8266Address, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
  }
  esp_now_register_send_cb(OnDataSent);
  
}

void loop() {
  Blynk.run();
  
  if(matchFace==true)
  {
    sendUnlockSignal();
    prevMillis=millis();
    Serial.print("UNLOCK DOOR"); 
    matchFace=false;   
  }
  delay(1000);


}
