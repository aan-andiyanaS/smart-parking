/*
 * Smart Parking System - Camera Only Version with MJPEG Streaming
 * 
 * This version uses only the camera for slot detection with live streaming.
 * Features:
 * - MJPEG live video streaming to dashboard
 * - Periodic YOLO detection every 5 seconds
 * - Real-time slot status updates via API
 * - No servo motor or ultrasonic sensors
 * 
 * Endpoints:
 * - GET /         - Status page
 * - GET /stream   - MJPEG video stream
 * - GET /capture  - Single frame capture
 * 
 * Author: Smart Parking Team
 * Updated: 2026-01-17
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ===========================================
// CONFIGURATION - EDIT THESE!
// ===========================================
const char* WIFI_SSID = "ANS030005";
const char* WIFI_PASSWORD = "71311311203";
const char* BACKEND_URL = "http://localhost:8080";      // Backend API
const char* AI_SERVICE_URL = "http://localhost:5000"; // AI service

// ===========================================
// PIN DEFINITIONS - ESP32-S3 WROOM + OV2640
// ===========================================

// Camera pins (OV2640 - built into module)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// LCD I2C pins
#define LCD_SDA           21
#define LCD_SCL           20

// Optional: LED indicators
#define LED_GREEN         4   // Available slot
#define LED_RED           5   // Full
#define LED_STATUS        2   // Status LED

// ===========================================
// CONSTANTS
// ===========================================
#define DETECTION_INTERVAL_MS   5000   // Run YOLO detection every 5 seconds
#define STATS_INTERVAL_MS       10000  // Fetch stats every 10 seconds
#define STREAM_FRAME_DELAY_MS   50     // ~20 FPS for stream

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

// MJPEG stream boundary
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================
bool initCamera();
void connectWiFi();
void setupServer();
void handleRoot();
void handleStream();
void handleCapture();
void handleStatus();
void handleRestart();
void runDetection();
void fetchStats();
void updateDisplay();
void updateLEDs();

// ===========================================
// SETUP
// ===========================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  Smart Parking - Camera Stream Version");
    Serial.println("  ESP32-S3 + OV2640 + MJPEG Streaming");
    Serial.println("========================================\n");
    
    // Initialize status LED
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, HIGH);
    
    // Initialize I2C for LCD
    Wire.begin(LCD_SDA, LCD_SCL);
    
    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Smart Parking");
    lcd.setCursor(0, 1);
    lcd.print("Stream v1.0");
    delay(2000);
    
    // Initialize LEDs
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Init Camera...");
    
    // Initialize Camera
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
    
    // Connect to WiFi
    lcd.clear();
    lcd.print("Connecting WiFi");
    connectWiFi();
    
    if (wifiConnected) {
        // Setup HTTP server
        setupServer();
        
        // Initial stats fetch
        fetchStats();
    }
    
    digitalWrite(LED_STATUS, LOW);
    updateDisplay();
    
    Serial.println("\n[READY] System is running!");
    Serial.printf("Stream URL: http://%s/stream\n", WiFi.localIP().toString().c_str());
    Serial.printf("Capture URL: http://%s/capture\n\n", WiFi.localIP().toString().c_str());
}

// ===========================================
// MAIN LOOP
// ===========================================
void loop() {
    // Handle HTTP requests
    server.handleClient();
    
    unsigned long currentTime = millis();
    
    // Periodic detection (only when not streaming to avoid lag)
    if (cameraReady && wifiConnected && !streamActive) {
        if (currentTime - lastDetectionTime > DETECTION_INTERVAL_MS) {
            runDetection();
            lastDetectionTime = currentTime;
        }
    }
    
    // Periodic stats fetch
    if (wifiConnected && (currentTime - lastStatsTime > STATS_INTERVAL_MS)) {
        fetchStats();
        lastStatsTime = currentTime;
    }
    
    // Update LED indicators
    updateLEDs();
    
    delay(10);
}

// ===========================================
// CAMERA INITIALIZATION
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
    
    // Use VGA for good quality streaming
    config.frame_size = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;            // 10-63, lower = better quality
    config.fb_count = 2;                 // Double buffer for smooth streaming
    config.grab_mode = CAMERA_GRAB_LATEST;
    
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        Serial.printf("[ERROR] Camera init failed: 0x%x\n", err);
        return false;
    }
    
    // Camera settings
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_brightness(s, 1);
        s->set_contrast(s, 1);
        s->set_saturation(s, 0);
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_exposure_ctrl(s, 1);
        s->set_aec2(s, 1);
    }
    
    return true;
}

// ===========================================
// HTTP SERVER SETUP
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

// Remote restart handler
void handleRestart() {
    Serial.println("[RESTART] Remote restart requested!");
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Restarting in 2 seconds...\"}");
    
    delay(2000);  // Give time for response to be sent
    ESP.restart();
}

// ===========================================
// HTTP HANDLERS
// ===========================================
void handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Smart Parking Camera</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;text-align:center;background:#1a1a2e;color:#fff;padding:20px}";
    html += "h1{color:#4ecca3}a{color:#4ecca3}.stream{max-width:100%;border-radius:10px;}</style>";
    html += "</head><body>";
    html += "<h1>🅿️ Smart Parking Camera</h1>";
    html += "<p>ESP32-S3 + OV2640</p>";
    html += "<img class='stream' src='/stream'><br><br>";
    html += "<p>Available: " + String(availableSlots) + "/" + String(totalSlots) + " slots</p>";
    html += "<p><a href='/capture'>📸 Capture Single Frame</a></p>";
    html += "<p><a href='/status'>📊 JSON Status</a></p>";
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
    
    Serial.println("[STREAM] Client connected");
    
    while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("[STREAM] Frame capture failed");
            continue;
        }
        
        client.print(STREAM_BOUNDARY);
        
        char partHeader[64];
        snprintf(partHeader, sizeof(partHeader), STREAM_PART, fb->len);
        client.print(partHeader);
        
        client.write(fb->buf, fb->len);
        
        esp_camera_fb_return(fb);
        
        delay(STREAM_FRAME_DELAY_MS);
    }
    
    streamActive = false;
    Serial.println("[STREAM] Client disconnected");
}

void handleCapture() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        server.send(500, "text/plain", "Camera capture failed");
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
    doc["stream_url"] = "http://" + WiFi.localIP().toString() + "/stream";
    doc["available_slots"] = availableSlots;
    doc["total_slots"] = totalSlots;
    doc["camera_ready"] = cameraReady;
    doc["stream_active"] = streamActive;
    doc["uptime_ms"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
}

// ===========================================
// DETECTION FUNCTION
// ===========================================
void runDetection() {
    Serial.println("\n[DETECT] Running YOLO detection...");
    digitalWrite(LED_STATUS, HIGH);
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[DETECT] Capture failed");
        digitalWrite(LED_STATUS, LOW);
        return;
    }
    
    Serial.printf("[DETECT] Image: %d bytes\n", fb->len);
    
    // Send to AI service
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(AI_SERVICE_URL) + "/analyze";
        
        http.begin(url);
        http.setTimeout(15000);
        
        // Create multipart form data
        String boundary = "----ESP32Boundary";
        String contentType = "multipart/form-data; boundary=" + boundary;
        http.addHeader("Content-Type", contentType);
        
        // Build multipart body
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
            
            int httpCode = http.POST(payload, totalLen);
            
            if (httpCode == 200) {
                String response = http.getString();
                Serial.println("[DETECT] AI response received");
                
                // Parse response
                DynamicJsonDocument doc(2048);
                if (deserializeJson(doc, response) == DeserializationError::Ok) {
                    if (doc["success"]) {
                        int vehicles = doc["vehicles_detected"] | 0;
                        Serial.printf("[DETECT] Detected %d vehicles\n", vehicles);
                        
                        // Stats will be updated via WebSocket from backend
                    }
                }
            } else {
                Serial.printf("[DETECT] AI error: %d\n", httpCode);
            }
            
            free(payload);
        }
        
        http.end();
    }
    
    esp_camera_fb_return(fb);
    digitalWrite(LED_STATUS, LOW);
}

// ===========================================
// STATS FETCH
// ===========================================
void fetchStats() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    String url = String(BACKEND_URL) + "/api/slots/stats";
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            if (doc["success"]) {
                totalSlots = doc["data"]["total"] | 4;
                occupiedSlots = doc["data"]["occupied"] | 0;
                availableSlots = totalSlots - occupiedSlots;
                
                Serial.printf("[STATS] %d/%d available\n", availableSlots, totalSlots);
                updateDisplay();
            }
        }
    }
    
    http.end();
}

// ===========================================
// WIFI CONNECTION
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
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        
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
// DISPLAY FUNCTIONS
// ===========================================
void updateDisplay() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PARKIR CERDAS");
    lcd.setCursor(0, 1);
    lcd.print("Slot: ");
    lcd.print(availableSlots);
    lcd.print("/");
    lcd.print(totalSlots);
    
    if (availableSlots == 0) {
        lcd.print(" PENUH");
    } else {
        lcd.print(" OK");
    }
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
