/*
 * Smart Parking System - Camera Only Version
 * For Arduino IDE
 * 
 * Board: ESP32-S3 Dev Module
 * Camera: OV2640
 * 
 * Features:
 * - MJPEG live video streaming
 * - YOLO detection via AI Service
 * - Real-time slot status updates
 * 
 * Endpoints:
 * - GET /         - Status page
 * - GET /stream   - MJPEG video stream
 * - GET /capture  - Single frame
 * - GET /status   - JSON status
 * - GET /restart  - Remote restart
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===========================================
// CONFIGURATION - EDIT THESE!
// ===========================================
const char* WIFI_SSID = "ANS030005";
const char* WIFI_PASSWORD = "71311311203";
const char* BACKEND_URL = "http://10.93.213.141:8080";
const char* AI_SERVICE_URL = "http://10.93.213.141:5000";

// ===========================================
// PIN DEFINITIONS - ESP32-S3 CAM
// ===========================================
// #define PWDN_GPIO_NUM     -1
// #define RESET_GPIO_NUM    -1
// #define XCLK_GPIO_NUM     10
// #define SIOD_GPIO_NUM     40
// #define SIOC_GPIO_NUM     39
// #define Y9_GPIO_NUM       48
// #define Y8_GPIO_NUM       11
// #define Y7_GPIO_NUM       12
// #define Y6_GPIO_NUM       14
// #define Y5_GPIO_NUM       16
// #define Y4_GPIO_NUM       18
// #define Y3_GPIO_NUM       17
// #define Y2_GPIO_NUM       15
// #define VSYNC_GPIO_NUM    38
// #define HREF_GPIO_NUM     47
// #define PCLK_GPIO_NUM     13

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

// LCD I2C pins
#define LCD_SDA           21
#define LCD_SCL           20

// LED indicators
#define LED_GREEN         4
#define LED_RED           5
#define LED_STATUS        2

// ===========================================
// CONSTANTS
// ===========================================
#define DETECTION_INTERVAL_MS   5000
#define STATS_INTERVAL_MS       10000
#define STREAM_FRAME_DELAY_MS   50

// ===========================================
// GLOBAL OBJECTS
// ===========================================
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State variables
int availableSlots = 0;
int totalSlots = 4;
int occupiedSlots = 0;
unsigned long lastDetectionTime = 0;
unsigned long lastStatsTime = 0;
bool cameraReady = false;
bool wifiConnected = false;
bool streamActive = false;

// MJPEG stream
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
  
  Serial.println("\n========================================");
  Serial.println("  Smart Parking - Camera Only");
  Serial.println("  ESP32-S3 + OV2640");
  Serial.println("========================================\n");
  
  // Initialize LED
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_STATUS, HIGH);
  
  // Initialize LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
  
  // Initialize Camera
  lcd.clear();
  lcd.print("Init Camera...");
  
  if (!initCamera()) {
    Serial.println("[ERROR] Camera init FAILED!");
    lcd.clear();
    lcd.print("Camera Error!");
    while (true) {
      digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
      delay(200);
    }
  }
  cameraReady = true;
  Serial.println("[OK] Camera initialized");
  lcd.clear();
  lcd.print("Camera OK!");
  delay(1000);
  
  // Connect WiFi
  lcd.clear();
  lcd.print("Connecting WiFi");
  connectWiFi();
  
  if (wifiConnected) {
    setupServer();
    fetchStats();
  }
  
  digitalWrite(LED_STATUS, LOW);
  updateDisplay();
  
  Serial.println("\n[READY] System running!");
  Serial.print("Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

// ===========================================
// LOOP
// ===========================================
void loop() {
  server.handleClient();
  
  unsigned long now = millis();
  
  // Detection (when not streaming)
  if (cameraReady && wifiConnected && !streamActive) {
    if (now - lastDetectionTime > DETECTION_INTERVAL_MS) {
      runDetection();
      lastDetectionTime = now;
    }
  }
  
  // Stats fetch
  if (wifiConnected && (now - lastStatsTime > STATS_INTERVAL_MS)) {
    fetchStats();
    lastStatsTime = now;
  }
  
  updateLEDs();
  delay(10);
}

// ===========================================
// CAMERA INIT
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
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init error: 0x%x\n", err);
    return false;
  }
  
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
  }
  
  return true;
}

// ===========================================
// WIFI
// ===========================================
void connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[OK] WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.print("WiFi OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println("\n[ERROR] WiFi failed!");
    lcd.clear();
    lcd.print("WiFi ERROR!");
    delay(2000);
  }
}

// ===========================================
// SERVER SETUP
// ===========================================
void setupServer() {
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/capture", handleCapture);
  server.on("/status", handleStatus);
  server.on("/restart", handleRestart);
  
  server.begin();
  Serial.println("[OK] HTTP server started");
}

// ===========================================
// HTTP HANDLERS
// ===========================================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Smart Parking</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;background:#1a1a2e;color:#fff;padding:20px}";
  html += "h1{color:#4ecca3}a{color:#4ecca3}img{max-width:100%;border-radius:10px;}</style>";
  html += "</head><body>";
  html += "<h1>Smart Parking Camera</h1>";
  html += "<img src='/stream'><br><br>";
  html += "<p>Slot: " + String(availableSlots) + "/" + String(totalSlots) + "</p>";
  html += "<p><a href='/capture'>Capture</a> | <a href='/status'>Status</a> | <a href='/restart'>Restart</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleStream() {
  streamActive = true;
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "Content-Type: " + String(STREAM_CONTENT_TYPE) + "\r\n\r\n";
  client.print(response);
  
  Serial.println("[STREAM] Started");
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;
    
    client.print(STREAM_BOUNDARY);
    char partHeader[64];
    snprintf(partHeader, sizeof(partHeader), STREAM_PART, fb->len);
    client.print(partHeader);
    client.write(fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
    delay(STREAM_FRAME_DELAY_MS);
  }
  
  streamActive = false;
  Serial.println("[STREAM] Ended");
}

void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["device"] = "ESP32-S3-CAM";
  doc["ip"] = WiFi.localIP().toString();
  doc["available"] = availableSlots;
  doc["total"] = totalSlots;
  doc["stream_active"] = streamActive;
  doc["uptime"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleRestart() {
  Serial.println("[RESTART] Requested");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
  delay(1000);
  ESP.restart();
}

// ===========================================
// DETECTION
// ===========================================
void runDetection() {
  Serial.println("[DETECT] Running...");
  digitalWrite(LED_STATUS, HIGH);
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    digitalWrite(LED_STATUS, LOW);
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(AI_SERVICE_URL) + "/analyze";
    
    http.begin(url);
    http.setTimeout(15000);
    
    String boundary = "----ESP32Boundary";
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    
    String bodyStart = "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\n";
    bodyStart += "Content-Type: image/jpeg\r\n\r\n";
    String bodyEnd = "\r\n--" + boundary + "--\r\n";
    
    size_t totalLen = bodyStart.length() + fb->len + bodyEnd.length();
    uint8_t *payload = (uint8_t*)malloc(totalLen);
    
    if (payload) {
      memcpy(payload, bodyStart.c_str(), bodyStart.length());
      memcpy(payload + bodyStart.length(), fb->buf, fb->len);
      memcpy(payload + bodyStart.length() + fb->len, bodyEnd.c_str(), bodyEnd.length());
      
      int code = http.POST(payload, totalLen);
      if (code == 200) {
        Serial.println("[DETECT] Success");
      } else {
        Serial.printf("[DETECT] Error: %d\n", code);
      }
      free(payload);
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
  String url = String(BACKEND_URL) + "/api/slots/stats";
  
  http.begin(url);
  http.setTimeout(5000);
  
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      if (doc["success"]) {
        totalSlots = doc["data"]["total"] | 4;
        occupiedSlots = doc["data"]["occupied"] | 0;
        availableSlots = totalSlots - occupiedSlots;
        Serial.printf("[STATS] %d/%d\n", availableSlots, totalSlots);
        updateDisplay();
      }
    }
  }
  http.end();
}

// ===========================================
// DISPLAY
// ===========================================
void updateDisplay() {
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
