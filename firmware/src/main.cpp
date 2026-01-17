/*
 * Smart Parking System - ESP32-S3 IoT Device
 * 
 * Hardware:
 * - ESP32-S3 WROOM N16R8 with OV2640 Camera
 * - Servo Motor (Gate Control)
 * - LCD 16x2 I2C Display
 * - VL53L0X ToF Sensor (Vehicle Detection)
 * 
 * Features:
 * - Capture parking area images
 * - Detect vehicle at gate entrance
 * - Control gate barrier
 * - Display status on LCD
 * - Send data to backend server
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <VL53L0X.h>

// ==========================================
// WiFi Configuration
// ==========================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ==========================================
// Server Configuration
// ==========================================
const char* SERVER_URL = "http://YOUR_SERVER_IP";  // e.g., "http://192.168.1.100"
const char* UPLOAD_ENDPOINT = "/api/capture";
const char* SLOTS_ENDPOINT = "/api/slots/stats";
const char* CAMERA_ID = "cam1";

// ==========================================
// Pin Definitions for ESP32-S3 Camera
// ==========================================
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

// ==========================================
// I2C Pins (for LCD and VL53L0X)
// ==========================================
#define I2C_SDA           1
#define I2C_SCL           2

// ==========================================
// Servo Pin
// ==========================================
#define SERVO_PIN         21

// ==========================================
// LED Indicators
// ==========================================
#define LED_GREEN         4
#define LED_RED           5
#define BUZZER_PIN        6

// ==========================================
// Global Objects
// ==========================================
Servo gateServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16 cols, 2 rows
VL53L0X tofSensor;

// ==========================================
// Configuration
// ==========================================
const int GATE_OPEN_ANGLE = 90;
const int GATE_CLOSED_ANGLE = 0;
const int VEHICLE_DETECT_DISTANCE = 200;  // mm
const unsigned long CAPTURE_INTERVAL = 10000;  // 10 seconds
const unsigned long CHECK_INTERVAL = 1000;     // 1 second

// ==========================================
// State Variables
// ==========================================
bool gateOpen = false;
int availableSlots = 0;
int totalSlots = 0;
unsigned long lastCaptureTime = 0;
unsigned long lastCheckTime = 0;

// ==========================================
// Function Declarations
// ==========================================
void setupCamera();
void setupWiFi();
void setupServo();
void setupLCD();
void setupToFSensor();
void setupLEDs();

bool captureAndUpload();
bool checkAvailableSlots();
void checkVehicleAtGate();
void openGate();
void closeGate();
void updateLCD();
void beepBuzzer(int times);

// ==========================================
// Setup
// ==========================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Smart Parking System ===");
    Serial.println("Initializing...");

    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // Setup all components
    setupLEDs();
    setupLCD();
    setupServo();
    setupToFSensor();
    setupCamera();
    setupWiFi();

    Serial.println("=== System Ready ===");
    updateLCD();
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
    unsigned long currentMillis = millis();

    // Capture and upload image periodically
    if (currentMillis - lastCaptureTime >= CAPTURE_INTERVAL) {
        lastCaptureTime = currentMillis;
        captureAndUpload();
    }

    // Check available slots periodically
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;
        checkAvailableSlots();
        checkVehicleAtGate();
        updateLCD();
    }

    delay(100);  // Small delay to prevent CPU overload
}

// ==========================================
// WiFi Setup
// ==========================================
void setupWiFi() {
    lcd.clear();
    lcd.print("Connecting WiFi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        lcd.setCursor(attempts % 16, 1);
        lcd.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        lcd.clear();
        lcd.print("WiFi Connected!");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        
        digitalWrite(LED_GREEN, HIGH);
        delay(1000);
        digitalWrite(LED_GREEN, LOW);
    } else {
        Serial.println("\nWiFi Failed!");
        lcd.clear();
        lcd.print("WiFi Failed!");
        
        digitalWrite(LED_RED, HIGH);
    }
    delay(2000);
}

// ==========================================
// Camera Setup
// ==========================================
void setupCamera() {
    Serial.println("Setting up camera...");

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
    config.frame_size = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        lcd.clear();
        lcd.print("Camera Error!");
        return;
    }

    Serial.println("Camera ready!");
}

// ==========================================
// Servo Setup
// ==========================================
void setupServo() {
    Serial.println("Setting up servo...");
    
    ESP32PWM::allocateTimer(1);
    gateServo.setPeriodHertz(50);
    gateServo.attach(SERVO_PIN, 500, 2400);
    gateServo.write(GATE_CLOSED_ANGLE);
    
    Serial.println("Servo ready!");
}

// ==========================================
// LCD Setup
// ==========================================
void setupLCD() {
    Serial.println("Setting up LCD...");
    
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("Smart Parking");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    Serial.println("LCD ready!");
}

// ==========================================
// ToF Sensor Setup
// ==========================================
void setupToFSensor() {
    Serial.println("Setting up ToF sensor...");
    
    tofSensor.setTimeout(500);
    
    if (!tofSensor.init()) {
        Serial.println("ToF sensor init failed!");
        lcd.clear();
        lcd.print("ToF Error!");
        return;
    }
    
    tofSensor.startContinuous();
    Serial.println("ToF sensor ready!");
}

// ==========================================
// LED Setup
// ==========================================
void setupLEDs() {
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
}

// ==========================================
// Capture and Upload Image
// ==========================================
bool captureAndUpload() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return false;
    }

    Serial.println("Capturing image...");

    // Capture image
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed!");
        return false;
    }

    Serial.printf("Image captured: %d bytes\n", fb->len);

    // Upload to server
    HTTPClient http;
    String url = String(SERVER_URL) + UPLOAD_ENDPOINT;
    
    http.begin(url);
    http.addHeader("Content-Type", "image/jpeg");

    // Create multipart form data
    String boundary = "----ESP32CAM";
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\n";
    head += "Content-Type: image/jpeg\r\n\r\n";
    
    String cameraIdPart = "\r\n--" + boundary + "\r\n";
    cameraIdPart += "Content-Disposition: form-data; name=\"camera_id\"\r\n\r\n";
    cameraIdPart += String(CAMERA_ID);
    
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t totalLen = head.length() + fb->len + cameraIdPart.length() + tail.length();
    
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http.addHeader("Content-Length", String(totalLen));

    // Create buffer
    uint8_t* buffer = (uint8_t*)malloc(totalLen);
    if (!buffer) {
        Serial.println("Failed to allocate buffer!");
        esp_camera_fb_return(fb);
        return false;
    }

    uint32_t offset = 0;
    memcpy(buffer + offset, head.c_str(), head.length());
    offset += head.length();
    memcpy(buffer + offset, fb->buf, fb->len);
    offset += fb->len;
    memcpy(buffer + offset, cameraIdPart.c_str(), cameraIdPart.length());
    offset += cameraIdPart.length();
    memcpy(buffer + offset, tail.c_str(), tail.length());

    int httpCode = http.POST(buffer, totalLen);
    
    free(buffer);
    esp_camera_fb_return(fb);
    http.end();

    if (httpCode == 200) {
        Serial.println("Image uploaded successfully!");
        return true;
    } else {
        Serial.printf("Upload failed! HTTP code: %d\n", httpCode);
        return false;
    }
}

// ==========================================
// Check Available Slots from Server
// ==========================================
bool checkAvailableSlots() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = String(SERVER_URL) + SLOTS_ENDPOINT;
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc["success"]) {
            totalSlots = doc["data"]["total"];
            int occupied = doc["data"]["occupied"];
            availableSlots = doc["data"]["available"];
            
            Serial.printf("Slots: %d/%d available\n", availableSlots, totalSlots);
        }
        
        http.end();
        return true;
    }
    
    http.end();
    return false;
}

// ==========================================
// Check Vehicle at Gate
// ==========================================
void checkVehicleAtGate() {
    int distance = tofSensor.readRangeContinuousMillimeters();
    
    if (tofSensor.timeoutOccurred()) {
        Serial.println("ToF sensor timeout!");
        return;
    }

    Serial.printf("Distance: %d mm\n", distance);

    // Vehicle detected
    if (distance < VEHICLE_DETECT_DISTANCE) {
        if (!gateOpen) {
            if (availableSlots > 0) {
                // Open gate
                Serial.println("Vehicle detected! Opening gate...");
                openGate();
            } else {
                // Parking full
                Serial.println("PARKING FULL!");
                digitalWrite(LED_RED, HIGH);
                beepBuzzer(3);
                
                lcd.clear();
                lcd.print("PARKIR PENUH!");
                lcd.setCursor(0, 1);
                lcd.print("Silakan Kembali");
                
                delay(3000);
                digitalWrite(LED_RED, LOW);
            }
        }
    } else {
        // No vehicle - close gate after delay
        if (gateOpen) {
            delay(5000);  // Wait 5 seconds before closing
            closeGate();
        }
    }
}

// ==========================================
// Gate Control
// ==========================================
void openGate() {
    Serial.println("Opening gate...");
    gateServo.write(GATE_OPEN_ANGLE);
    gateOpen = true;
    
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    beepBuzzer(1);
    
    lcd.clear();
    lcd.print("SELAMAT DATANG!");
    lcd.setCursor(0, 1);
    lcd.print("Silakan Masuk");
}

void closeGate() {
    Serial.println("Closing gate...");
    gateServo.write(GATE_CLOSED_ANGLE);
    gateOpen = false;
    
    digitalWrite(LED_GREEN, LOW);
}

// ==========================================
// Update LCD Display
// ==========================================
void updateLCD() {
    if (gateOpen) return;  // Don't update during gate operation
    
    lcd.clear();
    lcd.print("Smart Parking");
    lcd.setCursor(0, 1);
    
    if (availableSlots > 0) {
        lcd.printf("Tersedia: %d/%d", availableSlots, totalSlots);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
    } else {
        lcd.print("PENUH!");
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
    }
}

// ==========================================
// Buzzer Control
// ==========================================
void beepBuzzer(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}
