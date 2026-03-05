#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
// Pin config
#define BUZZER_PIN D6 // Pin cho buzzer
#define BUTTON_ENROLL_PIN D4 // Thay đổi chân cho nút đăng ký
#define BUTTON_DELETE_PIN D3 // Pin cho nút xóa
#define RX_PIN D9  // RX pin của ESP8266
#define TX_PIN D10  // TX pin của ESP8266
int relay = D5; // Pin cho relay (để kiểm soát khóa cửa)
int count_error = 0;
// Khởi tạo SoftwareSerial cho cảm biến vân tay
SoftwareSerial mySerial(D7, D8); // RX, TX

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Khai báo hàm hiển thị
void displayWaitFingerWithAnimation();
void displayWaitFinger();
void displayFingerOK();
void displayInvalidFinger();
void displayEnrollFinger();
void displayDeleteFinger();
void enrollFingerprint();
void deleteFingerprint();

unsigned long lastEnrollButtonPress = 0; // Thời điểm nhấn nút cuối
unsigned long lastDeleteButtonPress = 0; // Thời điểm nhấn nút xóa cuối
const unsigned long debounceDelay = 200; // Thời gian debounce
#define BLYNK_TEMPLATE_ID "TMPL6mus-ARyx"
#define BLYNK_TEMPLATE_NAME "SmartDoorN2"
#define BLYNK_AUTH_TOKEN "AFoEJrt_jCD4qfCTVUG-IP57tfOLLU4E"
const char* ssid = "H Tiến";
const char* password = "10102003";
const char* host = "script.google.com";
const int httpsPort = 443;  // Cổng HTTPS
 
// URL của Google Apps Script Web App
const String scriptURL = "https://script.google.com/macros/s/AKfycbzIpvFRcYCgVeUsNTO47Vqq9dgfm1MBqDu_3LsTdc4TvEYwqh-vsJntRjE8dGb3qIRA/exec";
String username;
#include <Blynk.h>
#include<BlynkSimpleEsp8266.h>
// WebSocket server and Relay pin
BlynkTimer timer;  

#include <espnow.h>
// ESP-NOW message structure
unsigned long previousMillis = 0;
const long interval = 1000;  // Thay đổi thời gian theo nhu cầu
bool relayState = false;
unsigned long relayOnTime = 0; // Thời gian bật relay
const unsigned long relayTimeout = 5000; // Thời gian relay bật (5 giây)
typedef struct struct_message {
    char ipAddress[16]; // Địa chỉ IP
    bool faceRecognized = false; // Biến để chỉ định khuôn mặt đã được nhận diện
} struct_message;

struct_message incomingData;
  // Tạo biến tạm để lưu dữ liệu nhận được
struct_message incomingDataTemp;

void onDataRecv(uint8_t *mac, uint8_t *incomingDataPtr, uint8_t len) {
    if (len != sizeof(struct_message)) {
        Serial.println("Invalid data length");
        return;
    }

    if (incomingDataPtr == NULL) {
        Serial.println("Null data pointer");
        return;
    }

    // Sao chép dữ liệu
    memcpy(&incomingDataTemp, incomingDataPtr, sizeof(incomingDataTemp));

    Serial.print("Received IP Address: ");
    Serial.println(incomingDataTemp.ipAddress);
    Serial.print("Face Recognized: ");
    Serial.println(incomingDataTemp.faceRecognized ? "Yes" : "No");

    // Nếu nhận diện đúng, bật relay
    if (incomingDataTemp.faceRecognized && !relayState) {
        digitalWrite(relay, HIGH);
        relayState = true;
        relayOnTime = millis();
        Serial.println("Relay ON (Face Recognized)");
    }
}

// Kiểm tra thời gian timeout của relay
void handleRelayTimeout() {
    if (relayState && (millis() - relayOnTime >= relayTimeout)) {
        digitalWrite(relay, LOW);
        relayState = false;
        Serial.println("Relay OFF (Timeout)");
    }
}
BLYNK_WRITE(V1) {  
  int pinValue = param.asInt(); // Lấy giá trị từ ứng dụng
  digitalWrite(relay, pinValue);
  Serial.println(pinValue ? "Relay ON" : "Relay OFF");
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
}
BLYNK_WRITE(V3) {  // Enroll fingerprint
  Serial.println("Enroll command received from Blynk app.");
  displayEnrollFinger();
  enrollFingerprint();
}

BLYNK_WRITE(V2) {  // Delete fingerprint
  Serial.println("Delete command received from Blynk app.");
  displayDeleteFinger();
  deleteFingerprint();
}

// Hàm lấy tên người dùng từ Google Sheets dựa trên IDfinger
String getUserName(int IDfinger) {
  WiFiClientSecure client;
  client.setInsecure();

  String url = scriptURL + "?sts=getName&fingerID=" + String(IDfinger);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  if (client.connect(host, httpsPort)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    while (client.connected() || client.available()) {
      if (client.available()) {
        String response = client.readStringUntil('\n');
        Serial.print("Received Name: ");
        Serial.println(response);
        return response;  // Trả về tên người dùng
      }
    }
  } else {
    Serial.println("Failed to connect to Google Apps Script");
  }
  return "Name not found";
}
void importID(int IDfinger){
  WiFiClientSecure client;
  client.setInsecure();

  String url = scriptURL + "?sts=writeName&id=" + String(IDfinger);
  Serial.print("Requesting URL for access log: ");
  Serial.println(url);

  if (client.connect(host, httpsPort)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "Connection: close\r\n\r\n");

    Serial.println("ID Finger was imported successfully.");
  } else {
    Serial.println("Failed to connect to Google Apps Script for logging");
  }
  }
void deleteID(int IDfinger) {
  WiFiClientSecure client;
  client.setInsecure();

  String url = scriptURL + "?sts=deleteID&id=" + String(IDfinger);
  Serial.print("Requesting URL for access log: ");
  Serial.println(url);

  if (client.connect(host, httpsPort)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("ID Finger was deleted successfully.");
  } else {
    Serial.println("Failed to connect to Google Apps Script for logging");
  }
}
// Hàm ghi nhật ký truy cập vào Google Sheets
void logAccess(int IDfinger) {
  WiFiClientSecure client;
  client.setInsecure();

  String url = scriptURL + "?sts=write&id=" + String(IDfinger);
  Serial.print("Requesting URL for access log: ");
  Serial.println(url);

  if (client.connect(host, httpsPort)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Access log written successfully.");
  } else {
    Serial.println("Failed to connect to Google Apps Script for logging");
  }
}
void setup() {
  Serial.begin(57600);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  mySerial.begin(57600); // Bắt đầu SoftwareSerial
  
  pinMode(relay, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_ENROLL_PIN, INPUT_PULLUP); // Chế độ kéo lên cho nút đăng ký
  pinMode(BUTTON_DELETE_PIN, INPUT_PULLUP); // Chế độ kéo lên cho nút xóa
  digitalWrite(relay, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  // Connect to Wi-Fi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  
  Serial.print("ESP8266 IP Address: ");
  Serial.println(WiFi.localIP());
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  // Khởi tạo màn hình OLED
  u8g2.begin();
  displayWaitFingerWithAnimation();

  Serial.println("\nAdafruit Fingerprint sensor setup for AS608!");

  // Bắt đầu cảm biến vân tay
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("Fingerprint sensor not found!");
    while (true) { delay(1); }
  }

  Serial.println("Reading sensor parameters");
  finger.getParameters();
  
  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.println("No fingerprint data found.");
  } else {
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  WiFi.mode(WIFI_STA); // Chế độ STA

  // Khởi tạo ESP-NOW
  if (esp_now_init() != 0) {
      Serial.println("Error initializing ESP-NOW");
      return;
  }

  // Đăng ký callback nhận dữ liệu
  //esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
  timer.setInterval(100L, handleRelayTimeout); // Gọi hàm kiểm tra timeout

}

void loop() {
  
  Blynk.run();
  timer.run();
  digitalWrite(BUZZER_PIN, HIGH);
  // Kiểm tra trạng thái của nút nhấn với debounce
  int enrollButtonState = digitalRead(BUTTON_ENROLL_PIN);
  int deleteButtonState = digitalRead(BUTTON_DELETE_PIN);
 
  unsigned long currentMillis = millis(); // Lấy thời gian hiện tại
  
    // Kiểm tra nút đăng ký
    if (enrollButtonState == LOW && (currentMillis - lastEnrollButtonPress) > debounceDelay) {
      lastEnrollButtonPress = currentMillis; // Cập nhật thời gian nhấn nút
      displayEnrollFinger();
      Serial.println("Adding...");
      enrollFingerprint();
    }

    // Kiểm tra nút xóa
    if (deleteButtonState == LOW && (currentMillis - lastDeleteButtonPress) > debounceDelay) {
      lastDeleteButtonPress = currentMillis; // Cập nhật thời gian nhấn nút
    
      displayDeleteFinger();
      delay(1000);
      deleteFingerprint();
      Serial.println("Deleted");
    }

    // Màn hình chờ
    // displayWaitFingerWithAnimation();
    //delay(5000);
    displayIPAddress(incomingDataTemp.ipAddress);
    //delay(5000);

    int fingerprintID = getFingerprintIDez();
    if (fingerprintID == -1) {
      getFingerprintID();
    } else {
      Serial.println("===========================");
      Serial.println("Unlocking door...");
      digitalWrite(relay, HIGH);
      displayFingerOK();
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      //digitalWrite(BUZZER_PIN, HIGH); // Buzzer sound on success
      delay(4800);

      Serial.println("Locking door...");
      digitalWrite(relay, LOW);
      //digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
      displayWaitFingerWithAnimation();
      if (username != "Name not found") {
        logAccess(int(finger.fingerID));  // Ghi lại nhật ký truy cập nếu tên tồn tại
        Serial.println("Door opened for authorized user.");
      }
    }
  delay(50);
    
}

//=========================Fingerprint Functions==========================
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Found ID #"); Serial.print(finger.fingerID);
    Serial.print(" with confidence of "); Serial.println(finger.confidence);
    
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Fingerprint not found");
    displayInvalidFinger();
    count_error++;
    Blynk.virtualWrite(V5, count_error);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    if(count_error == 3){
      digitalWrite(BUZZER_PIN, LOW);
      delay(3000);
      digitalWrite(BUZZER_PIN, HIGH);
      count_error = 0;
      Blynk.virtualWrite(V5, count_error);

    }
    delay(2000);
    return p;
  }

  return finger.fingerID;
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

//======================= OLED Display Functions ========================
void displayWaitFinger() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Place your Finger");
  u8g2.sendBuffer();
}

void displayFingerOK() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Welcome!");
  u8g2.sendBuffer();
}

void displayInvalidFinger() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "No Match!");
  u8g2.sendBuffer();
}

void displayEnrollFinger() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Enroll Finger");
  u8g2.sendBuffer();
}

void displayDeleteFinger() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Delete Finger");
  u8g2.sendBuffer();
}

void displayWaitFingerWithAnimation() {
  static int pos = 0;  // Vị trí cho thanh tiến trình
  u8g2.clearBuffer();
  
  // Text hướng dẫn
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 20, "Waiting for");
  u8g2.drawStr(15, 40, "your finger");

  // Vẽ thanh tiến trình
  u8g2.drawFrame(0, 55, 128, 8); // Khung cho progress bar
  u8g2.drawBox(pos, 55, 20, 8);  // Hình chữ nhật động
  pos += 5;
  if (pos > 128) {
    pos = 0;  // Quay lại từ đầu khi quá chiều dài màn hình
  }

  u8g2.sendBuffer();
  delay(200);  // Tạo độ trễ để thanh tiến trình di chuyển
}
void displayIPAddress(const char* ipAddress) {
    if (ipAddress == NULL) {
        Serial.println("Error: IP Address is NULL");
        return;
    }

    u8g2.clearBuffer(); // Xóa bộ đệm

    // Chọn font chữ rõ ràng
    u8g2.setFont(u8g2_font_ncenB08_tr); 

    // Tính toán vị trí để căn giữa thông điệp
    const char* message = "Access IP to CAM:";
    int16_t messageWidth = u8g2.getStrWidth(message);
    int16_t ipWidth = u8g2.getStrWidth(ipAddress);

    // Hiển thị thông điệp và địa chỉ IP, căn giữa theo chiều ngang
    u8g2.drawStr((u8g2.getDisplayWidth() - messageWidth) / 2, 30, message);
    u8g2.drawStr((u8g2.getDisplayWidth() - ipWidth) / 2, 45, ipAddress);

    // Gửi bộ đệm đến màn hình
    u8g2.sendBuffer(); 
}



void enrollFingerprint() {
  finger.templateCount++;
  int id = finger.templateCount;  // Assign new ID for the fingerprint
  Serial.print("Enrolling ID #"); Serial.println(id);

  for (int attempt = 1; attempt <= 2; attempt++) {
    Serial.println("Place finger on the sensor...");
    while (finger.getImage() != FINGERPRINT_OK);
    if (finger.image2Tz(attempt) != FINGERPRINT_OK) {
      Serial.println("Error capturing fingerprint, try again.");
      return;
    }
    delay(1000);
  }

  if (finger.createModel() == FINGERPRINT_OK) {
    if (finger.storeModel(id) == FINGERPRINT_OK) {
      Serial.println("Fingerprint enrolled successfully!");
      displayEnrollFinger();
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      if(int(finger.fingerID) > 0){
        username = getUserName(int(finger.fingerID));
      }
      importID(id);
      Blynk.virtualWrite(V3, 0);
    } else {
      Serial.println("Error storing fingerprint.");
    }
  } else {
    Serial.println("Error creating fingerprint model.");
    finger.templateCount--;
  }
}

void deleteFingerprint() {
  Serial.println("Deleting fingerprint...");
  
  // Kiểm tra xem có dấu vân tay nào đã được lưu không
  if (finger.templateCount > 0) {
    // Bạn có thể yêu cầu người dùng nhập ID dấu vân tay để xóa
    int id = finger.templateCount;  // Lấy ID của dấu vân tay cuối cùng
    Serial.print("Deleting fingerprint ID: "); Serial.println(id);
     deleteID(id);
    // Xóa dấu vân tay
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
      finger.templateCount --;

      Serial.println("Fingerprint deleted successfully!");
      displayEnrollFinger();
      Blynk.virtualWrite(V2, 0);
      digitalWrite(BUZZER_PIN, LOW);
      delay(300);
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      Serial.println("Error deleting fingerprint.");
    }
  } else {
    Serial.println("No fingerprints to delete.");
  }
 
}


