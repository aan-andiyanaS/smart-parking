/*
 * Smart Parking System - ESP32-S3 WROOM N16R8 + OV2640
 * 
 * Components:
 * - ESP32-S3 WROOM N16R8 with OV2640 Camera
 * - 2x HC-SR04 Ultrasonic Sensors (Entry & Exit)
 * - 1x SG90 Servo Motor (Gate)
 * - 1x LCD 16x2 I2C
 * 
 * Author: Smart Parking Team
 * Updated: 2026-01-17
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

// ===========================================
// CONFIGURATION - EDIT THESE!
// ===========================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL = "http://YOUR_BACKEND_IP";  // e.g., "http://192.168.1.100" or "http://api.yourdomain.com"

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

// Servo pin (Gate)
#define SERVO_PIN         14

// Ultrasonic Sensor 1 - ENTRY (Gate Masuk)
#define US1_TRIG_PIN      1
#define US1_ECHO_PIN      2

// Ultrasonic Sensor 2 - EXIT (Gate Keluar)
#define US2_TRIG_PIN      42
#define US2_ECHO_PIN      41

// Optional: LED indicators
#define LED_GREEN         4   // Available slot
#define LED_RED           5   // Full

// ===========================================
// CONSTANTS
// ===========================================
#define DETECTION_DISTANCE_CM   30    // Distance threshold for vehicle detection
#define GATE_OPEN_ANGLE         90    // Servo angle when gate is open
#define GATE_CLOSE_ANGLE        0     // Servo angle when gate is closed
#define GATE_OPEN_DURATION_MS   5000  // How long gate stays open
#define CAPTURE_INTERVAL_MS     10000 // Capture image every 10 seconds
#define STATS_INTERVAL_MS       5000  // Fetch stats every 5 seconds

// ===========================================
// GLOBAL OBJECTS
// ===========================================
Servo gateServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16 chars, 2 lines

// State variables
int availableSlots = 0;
int totalSlots = 4;
bool gateOpen = false;
unsigned long lastCaptureTime = 0;
unsigned long lastStatsTime = 0;
unsigned long gateOpenTime = 0;

// ===========================================
// SETUP
// ===========================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Smart Parking System ===");
    Serial.println("ESP32-S3 + OV2640 + 2x Ultrasonic");
    
    // Initialize I2C for LCD
    Wire.begin(LCD_SDA, LCD_SCL);
    
    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Smart Parking");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    // Initialize Servo
    gateServo.attach(SERVO_PIN);
    gateServo.write(GATE_CLOSE_ANGLE);
    Serial.println("Servo initialized (Gate closed)");
    
    // Initialize Ultrasonic sensors
    pinMode(US1_TRIG_PIN, OUTPUT);
    pinMode(US1_ECHO_PIN, INPUT);
    pinMode(US2_TRIG_PIN, OUTPUT);
    pinMode(US2_ECHO_PIN, INPUT);
    Serial.println("Ultrasonic sensors initialized");
    
    // Initialize LEDs (optional)
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    
    // Initialize Camera
    if (!initCamera()) {
        Serial.println("Camera init FAILED!");
        lcd.clear();
        lcd.print("Camera Error!");
        while (true) delay(1000);
    }
    Serial.println("Camera initialized");
    
    // Connect to WiFi
    connectWiFi();
    
    // Initial stats fetch
    fetchStats();
    
    // Display ready
    updateDisplay();
    Serial.println("=== System Ready ===\n");
}

// ===========================================
// MAIN LOOP
// ===========================================
void loop() {
    // Check Entry sensor (vehicle wants to enter)
    float entryDistance = measureDistance(US1_TRIG_PIN, US1_ECHO_PIN);
    if (entryDistance > 0 && entryDistance < DETECTION_DISTANCE_CM) {
        handleEntry();
    }
    
    // Check Exit sensor (vehicle wants to exit)
    float exitDistance = measureDistance(US2_TRIG_PIN, US2_ECHO_PIN);
    if (exitDistance > 0 && exitDistance < DETECTION_DISTANCE_CM) {
        handleExit();
    }
    
    // Auto-close gate after timeout
    if (gateOpen && (millis() - gateOpenTime > GATE_OPEN_DURATION_MS)) {
        closeGate();
    }
    
    // Periodic image capture
    if (millis() - lastCaptureTime > CAPTURE_INTERVAL_MS) {
        captureAndUpload();
        lastCaptureTime = millis();
    }
    
    // Periodic stats fetch
    if (millis() - lastStatsTime > STATS_INTERVAL_MS) {
        fetchStats();
        lastStatsTime = millis();
    }
    
    // Update LED indicators
    updateLEDs();
    
    delay(100);
}

// ===========================================
// ULTRASONIC SENSOR FUNCTIONS
// ===========================================
float measureDistance(int trigPin, int echoPin) {
    // Send pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Measure echo
    long duration = pulseIn(echoPin, HIGH, 30000);  // 30ms timeout
    
    if (duration == 0) {
        return -1;  // No echo received
    }
    
    // Calculate distance in cm
    float distance = duration * 0.034 / 2;
    return distance;
}

// ===========================================
// GATE CONTROL FUNCTIONS
// ===========================================
void handleEntry() {
    Serial.println("Vehicle detected at ENTRY");
    
    if (availableSlots > 0) {
        // Open gate for entry
        openGate();
        
        // Buzzer beep (success)
        beep(1);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SELAMAT DATANG!");
        lcd.setCursor(0, 1);
        lcd.print("Silakan Masuk");
        
        delay(3000);
        updateDisplay();
    } else {
        // Parking full
        beep(3);  // Error beep
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MAAF!");
        lcd.setCursor(0, 1);
        lcd.print("PARKIR PENUH!");
        
        delay(3000);
        updateDisplay();
    }
}

void handleExit() {
    Serial.println("Vehicle detected at EXIT");
    
    // Open gate for exit (always allow exit)
    openGate();
    
    // Buzzer beep (success)
    beep(1);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TERIMA KASIH!");
    lcd.setCursor(0, 1);
    lcd.print("Hati-hati...");
    
    delay(3000);
    updateDisplay();
}

void openGate() {
    if (!gateOpen) {
        Serial.println("Opening gate...");
        gateServo.write(GATE_OPEN_ANGLE);
        gateOpen = true;
        gateOpenTime = millis();
    }
}

void closeGate() {
    if (gateOpen) {
        Serial.println("Closing gate...");
        gateServo.write(GATE_CLOSE_ANGLE);
        gateOpen = false;
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

void beep(int times) {
    // If you have a buzzer connected
    // for (int i = 0; i < times; i++) {
    //     digitalWrite(BUZZER_PIN, HIGH);
    //     delay(100);
    //     digitalWrite(BUZZER_PIN, LOW);
    //     delay(100);
    // }
}

// ===========================================
// CAMERA FUNCTIONS
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
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_LATEST;
    
    esp_err_t err = esp_camera_init(&config);
    return (err == ESP_OK);
}

void captureAndUpload() {
    Serial.println("Capturing image...");
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    
    // Upload to server
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String uploadUrl = String(SERVER_URL) + "/api/capture";
        
        http.begin(uploadUrl);
        http.addHeader("Content-Type", "image/jpeg");
        
        int httpCode = http.POST(fb->buf, fb->len);
        
        if (httpCode > 0) {
            Serial.printf("Image uploaded, response: %d\n", httpCode);
        } else {
            Serial.printf("Upload failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        
        http.end();
    }
    
    esp_camera_fb_return(fb);
}

// ===========================================
// NETWORK FUNCTIONS
// ===========================================
void connectWiFi() {
    Serial.print("Connecting to WiFi");
    lcd.clear();
    lcd.print("Connecting WiFi");
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        lcd.clear();
        lcd.print("WiFi OK!");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        delay(2000);
    } else {
        Serial.println("\nWiFi connection failed!");
        lcd.clear();
        lcd.print("WiFi FAILED!");
    }
}

void fetchStats() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    HTTPClient http;
    String statsUrl = String(SERVER_URL) + "/api/slots/stats";
    
    http.begin(statsUrl);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc["success"]) {
            totalSlots = doc["data"]["total"] | 4;
            int occupied = doc["data"]["occupied"] | 0;
            availableSlots = totalSlots - occupied;
            
            Serial.printf("Stats: %d/%d slots available\n", availableSlots, totalSlots);
            updateDisplay();
        }
    }
    
    http.end();
}
