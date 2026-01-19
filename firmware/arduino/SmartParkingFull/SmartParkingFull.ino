/*
 * Smart Parking System - FULL VERSION
 * For Arduino IDE
 * 
 * Board: ESP32-S3 Dev Module
 * 
 * Components:
 * - Camera OV2640 (MJPEG streaming)
 * - LCD 16x2 I2C (status display)
 * - Servo Motor (gate control)
 * - VL53L0X ToF Sensor (vehicle detection)
 * - LEDs + Buzzer
 * 
 * Library yang diperlukan:
 * 1. ESP32Servo by Kevin Harrington
 * 2. LiquidCrystal I2C by Frank de Brabander
 * 3. VL53L0X by Pololu
 * 4. ArduinoJson by Benoit Blanchon
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <VL53L0X.h>

// ===========================================
// CONFIGURATION - EDIT THESE!
// ===========================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BACKEND_URL = "http://YOUR_BACKEND_IP:8080";
const char* AI_SERVICE_URL = "http://YOUR_AI_SERVICE_IP:5000";

// ===========================================
// PIN DEFINITIONS - Freenove ESP32-S3 WROOM CAM
// ===========================================
#define PWDN_GPIO_NUM     -1    // Power down tidak digunakan
#define RESET_GPIO_NUM    -1    // Reset tidak digunakan
#define XCLK_GPIO_NUM     15    // External clock
#define SIOD_GPIO_NUM     4     // I2C SDA (camera control)
#define SIOC_GPIO_NUM     5     // I2C SCL (camera control)

#define Y9_GPIO_NUM       16    // Data bit 7
#define Y8_GPIO_NUM       17    // Data bit 6
#define Y7_GPIO_NUM       18    // Data bit 5
#define Y6_GPIO_NUM       12    // Data bit 4
#define Y5_GPIO_NUM       10    // Data bit 3
#define Y4_GPIO_NUM       8     // Data bit 2
#define Y3_GPIO_NUM       9     // Data bit 1
#define Y2_GPIO_NUM       11    // Data bit 0

#define VSYNC_GPIO_NUM    6     // Vertical sync
#define HREF_GPIO_NUM     7     // Horizontal reference
#define PCLK_GPIO_NUM     13    // Pixel clock

// I2C Pins (LCD & ToF)
#define I2C_SDA           1
#define I2C_SCL           2

// Other Pins
#define SERVO_PIN         21
#define LED_GREEN         4
#define LED_RED           5
#define LED_STATUS        6
#define BUZZER_PIN        7

// Ultrasonic Sensors (Entry & Exit) - for single gate with direction detection
// Pins sesuai dengan ESP32-S3 WROOM N16R8 (lihat WIRING_GUIDE.md)
#define US_ENTRY_TRIG     1   // GPIO 1
#define US_ENTRY_ECHO     2   // GPIO 2
#define US_EXIT_TRIG      42  // GPIO 42
#define US_EXIT_ECHO      41  // GPIO 41

// ===========================================
// CONSTANTS
// ===========================================
#define GATE_OPEN_ANGLE       90
#define GATE_CLOSED_ANGLE     0
#define VEHICLE_DETECT_MM     200
#define VEHICLE_DETECT_CM     30    // For ultrasonic sensors
#define DETECTION_INTERVAL_MS 5000
#define STATS_INTERVAL_MS     5000
#define STREAM_FRAME_DELAY_MS 50
#define SENSOR_COOLDOWN_MS    5000  // Cooldown between entry/exit sensor

// ===========================================
// GLOBAL OBJECTS
// ===========================================
WebServer server(80);
Servo gateServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
VL53L0X tofSensor;

// State
int availableSlots = 0;
int totalSlots = 4;
bool gateOpen = false;
bool streamActive = false;
unsigned long lastDetectionTime = 0;
unsigned long lastStatsTime = 0;
unsigned long gateOpenTime = 0;

// Sensor cooldown state (for single gate setup)
unsigned long entrySensorCooldownUntil = 0;
unsigned long exitSensorCooldownUntil = 0;

// Stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ===========================================
// SETUP
// ===========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Smart Parking - FULL ===");
  
  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_STATUS, HIGH);
  
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Full Version");
  delay(1000);
  
  // Servo
  ESP32PWM::allocateTimer(1);
  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, 500, 2400);
  gateServo.write(GATE_CLOSED_ANGLE);
  Serial.println("[OK] Servo");
  
  // ToF Sensor (optional, can be used as backup)
  lcd.clear();
  lcd.print("Init Sensors...");
  tofSensor.setTimeout(500);
  if (tofSensor.init()) {
    tofSensor.startContinuous();
    Serial.println("[OK] ToF Sensor");
  } else {
    Serial.println("[WARN] ToF failed (using ultrasonic)");
  }
  
  // Ultrasonic Sensors (Entry & Exit)
  pinMode(US_ENTRY_TRIG, OUTPUT);
  pinMode(US_ENTRY_ECHO, INPUT);
  pinMode(US_EXIT_TRIG, OUTPUT);
  pinMode(US_EXIT_ECHO, INPUT);
  Serial.println("[OK] Ultrasonic Sensors");
  
  // Camera
  lcd.clear();
  lcd.print("Init Camera...");
  if (!initCamera()) {
    lcd.print("Camera Error!");
    while(1) { delay(1000); }
  }
  Serial.println("[OK] Camera");
  
  // WiFi
  lcd.clear();
  lcd.print("Connecting WiFi");
  connectWiFi();
  
  // HTTP Server
  setupServer();
  
  digitalWrite(LED_STATUS, LOW);
  updateLCD();
  
  Serial.println("\n=== READY ===");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

// ===========================================
// LOOP
// ===========================================
void loop() {
  server.handleClient();
  unsigned long now = millis();
  
  // Vehicle detection at gate
  checkVehicleAtGate();
  
  // Auto close gate
  if (gateOpen) {
    static unsigned long gateOpenTime = 0;
    if (gateOpenTime == 0) gateOpenTime = now;
    if (now - gateOpenTime > 5000) {
      closeGate();
      gateOpenTime = 0;
    }
  }
  
  // YOLO detection (when not streaming)
  if (!streamActive && (now - lastDetectionTime > DETECTION_INTERVAL_MS)) {
    runDetection();
    lastDetectionTime = now;
  }
  
  // Stats
  if (now - lastStatsTime > STATS_INTERVAL_MS) {
    fetchStats();
    lastStatsTime = now;
  }
  
  updateLEDs();
  delay(10);
}

// ===========================================
// CAMERA
// ===========================================
bool initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  return esp_camera_init(&config) == ESP_OK;
}

// ===========================================
// WIFI
// ===========================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi");
    lcd.clear();
    lcd.print("WiFi OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("[ERROR] WiFi");
    lcd.clear();
    lcd.print("WiFi ERROR!");
  }
}

// ===========================================
// HTTP SERVER
// ===========================================
void setupServer() {
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/capture", handleCapture);
  server.on("/status", handleStatus);
  server.on("/restart", handleRestart);
  server.on("/gate/open", []() {
    openGate();
    server.send(200, "application/json", "{\"success\":true}");
  });
  server.on("/gate/close", []() {
    closeGate();
    server.send(200, "application/json", "{\"success\":true}");
  });
  server.begin();
  Serial.println("[OK] Server");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Smart Parking</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;background:#1a1a2e;color:#fff;padding:20px}";
  html += "h1{color:#4ecca3}a{color:#4ecca3}img{max-width:100%;border-radius:10px;}";
  html += ".btn{background:#4ecca3;color:#1a1a2e;padding:10px 20px;border:none;border-radius:5px;margin:5px;cursor:pointer;text-decoration:none}</style>";
  html += "</head><body>";
  html += "<h1>Smart Parking - Full</h1>";
  html += "<img src='/stream'><br><br>";
  html += "<p>Slot: " + String(availableSlots) + "/" + String(totalSlots) + "</p>";
  html += "<p>Gate: " + String(gateOpen ? "OPEN" : "CLOSED") + "</p>";
  html += "<a class='btn' href='/gate/open'>Open Gate</a>";
  html += "<a class='btn' href='/gate/close'>Close Gate</a><br><br>";
  html += "<a href='/capture'>Capture</a> | <a href='/status'>Status</a> | <a href='/restart'>Restart</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStream() {
  streamActive = true;
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n";
  response += "Content-Type: " + String(STREAM_CONTENT_TYPE) + "\r\n\r\n";
  client.print(response);
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;
    
    client.print(STREAM_BOUNDARY);
    char h[64];
    snprintf(h, sizeof(h), STREAM_PART, fb->len);
    client.print(h);
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    delay(STREAM_FRAME_DELAY_MS);
  }
  streamActive = false;
}

void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { server.send(500, "text/plain", "Error"); return; }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["device"] = "ESP32-S3-Full";
  doc["ip"] = WiFi.localIP().toString();
  doc["available"] = availableSlots;
  doc["total"] = totalSlots;
  doc["gate"] = gateOpen ? "open" : "closed";
  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleRestart() {
  server.send(200, "application/json", "{\"success\":true}");
  delay(1000);
  ESP.restart();
}

// ===========================================
// GATE CONTROL
// ===========================================

// Measure distance with ultrasonic sensor
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

void checkVehicleAtGate() {
  unsigned long now = millis();
  
  // Check Entry sensor (only if not in cooldown)
  if (now > exitSensorCooldownUntil) {
    float entryDistance = measureDistance(US_ENTRY_TRIG, US_ENTRY_ECHO);
    if (entryDistance > 0 && entryDistance < VEHICLE_DETECT_CM && !gateOpen) {
      if (availableSlots > 0) {
        openGate();
        sendEntryEvent();
        // Set cooldown: disable exit sensor
        exitSensorCooldownUntil = now + SENSOR_COOLDOWN_MS;
        Serial.printf("[SENSOR] Entry triggered, exit disabled for %d ms\n", SENSOR_COOLDOWN_MS);
      } else {
        beep(3);
        lcd.clear();
        lcd.print("PARKIR PENUH!");
        delay(2000);
        updateLCD();
      }
    }
  }
  
  // Check Exit sensor (only if not in cooldown)
  if (now > entrySensorCooldownUntil) {
    float exitDistance = measureDistance(US_EXIT_TRIG, US_EXIT_ECHO);
    if (exitDistance > 0 && exitDistance < VEHICLE_DETECT_CM && !gateOpen) {
      openGateForExit();
      sendExitEvent();
      // Set cooldown: disable entry sensor
      entrySensorCooldownUntil = now + SENSOR_COOLDOWN_MS;
      Serial.printf("[SENSOR] Exit triggered, entry disabled for %d ms\n", SENSOR_COOLDOWN_MS);
    }
  }
  
  // Auto close gate
  if (gateOpen && gateOpenTime > 0 && (now - gateOpenTime > 5000)) {
    closeGate();
    gateOpenTime = 0;
  }
}

void openGate() {
  gateServo.write(GATE_OPEN_ANGLE);
  gateOpen = true;
  gateOpenTime = millis();
  beep(1);
  lcd.clear();
  lcd.print("SELAMAT DATANG!");
  lcd.setCursor(0, 1);
  lcd.print("Silakan Masuk");
  Serial.println("[GATE] Opened for ENTRY");
}

void openGateForExit() {
  gateServo.write(GATE_OPEN_ANGLE);
  gateOpen = true;
  gateOpenTime = millis();
  beep(1);
  lcd.clear();
  lcd.print("TERIMA KASIH!");
  lcd.setCursor(0, 1);
  lcd.print("Hati-hati...");
  Serial.println("[GATE] Opened for EXIT");
}

void closeGate() {
  gateServo.write(GATE_CLOSED_ANGLE);
  gateOpen = false;
  updateLCD();
  Serial.println("[GATE] Closed");
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ===========================================
// AI DETECTION
// ===========================================
void runDetection() {
  digitalWrite(LED_STATUS, HIGH);
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { digitalWrite(LED_STATUS, LOW); return; }
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(AI_SERVICE_URL) + "/analyze");
    http.setTimeout(15000);
    
    String boundary = "----ESP32";
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    
    String bodyStart = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"c.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String bodyEnd = "\r\n--" + boundary + "--\r\n";
    
    size_t len = bodyStart.length() + fb->len + bodyEnd.length();
    uint8_t *buf = (uint8_t*)malloc(len);
    if (buf) {
      memcpy(buf, bodyStart.c_str(), bodyStart.length());
      memcpy(buf + bodyStart.length(), fb->buf, fb->len);
      memcpy(buf + bodyStart.length() + fb->len, bodyEnd.c_str(), bodyEnd.length());
      http.POST(buf, len);
      free(buf);
    }
    http.end();
  }
  esp_camera_fb_return(fb);
  digitalWrite(LED_STATUS, LOW);
}

// ===========================================
// STATS
// ===========================================
void fetchStats() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/api/slots/stats");
  if (http.GET() == 200) {
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, http.getString()) && doc["success"]) {
      totalSlots = doc["data"]["total"] | 4;
      int occ = doc["data"]["occupied"] | 0;
      availableSlots = totalSlots - occ;
      updateLCD();
    }
  }
  http.end();
}

// ===========================================
// DISPLAY
// ===========================================
void updateLCD() {
  if (gateOpen) return;
  lcd.clear();
  lcd.print("PARKIR CERDAS");
  lcd.setCursor(0, 1);
  lcd.print("Slot: ");
  lcd.print(availableSlots);
  lcd.print("/");
  lcd.print(totalSlots);
  if (availableSlots == 0) lcd.print(" PENUH");
}

void updateLEDs() {
  if (availableSlots > 0) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }
}

// ===========================================
// SESSION TRACKING - Entry/Exit Events
// ===========================================
void sendEntryEvent() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SESSION] WiFi not connected, skipping entry event");
    return;
  }
  
  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/api/sessions/entry");
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  StaticJsonDocument<128> doc;
  doc["camera_id"] = "esp32-main";
  doc["event_type"] = "entry";
  
  String body;
  serializeJson(doc, body);
  
  int httpCode = http.POST(body);
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.println("[SESSION] Entry event sent successfully");
  } else {
    Serial.printf("[SESSION] Entry event failed: %d\n", httpCode);
  }
  
  http.end();
}

void sendExitEvent() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SESSION] WiFi not connected, skipping exit event");
    return;
  }
  
  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/api/sessions/exit");
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  StaticJsonDocument<128> doc;
  doc["camera_id"] = "esp32-main";
  doc["event_type"] = "exit";
  
  String body;
  serializeJson(doc, body);
  
  int httpCode = http.POST(body);
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.println("[SESSION] Exit event sent successfully");
  } else {
    Serial.printf("[SESSION] Exit event failed: %d\n", httpCode);
  }
  
  http.end();
}
