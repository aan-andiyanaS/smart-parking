/*
 * Smart Parking System - Blynk Version (No Camera)
 * For Arduino IDE with ESP32-S3
 * 
 * Board: ESP32-S3 Dev Module
 * 
 * Components:
 * - LCD 16x2 I2C (status display)
 * - Servo Motor (gate control)
 * - 2x HC-SR04 Ultrasonic Sensors (Entry & Exit)
 * - LEDs + Buzzer
 * 
 * Features:
 * - Entry/Exit detection with cooldown
 * - Gate control with servo
 * - Real-time monitoring via Blynk app
 * - Session tracking (entry/exit count)
 * 
 * Library yang diperlukan:
 * 1. Blynk by Volodymyr Shymanskyy
 * 2. ESP32Servo by Kevin Harrington
 * 3. LiquidCrystal I2C by Frank de Brabander
 */

#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Parking"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

// Uncomment untuk debug via Serial
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===========================================
// CONFIGURATION - EDIT THESE!
// ===========================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Total parking slots
const int TOTAL_SLOTS = 4;

// ===========================================
// PIN DEFINITIONS - ESP32-S3 WROOM N16R8
// ===========================================

// I2C Pins (LCD)
#define I2C_SDA           21
#define I2C_SCL           20

// Servo (Gate)
#define SERVO_PIN         14

// Ultrasonic Sensors (Entry & Exit)
#define US_ENTRY_TRIG     1
#define US_ENTRY_ECHO     2
#define US_EXIT_TRIG      42
#define US_EXIT_ECHO      41

// LEDs & Buzzer
#define LED_GREEN         4
#define LED_RED           5
#define BUZZER_PIN        7

// ===========================================
// CONSTANTS
// ===========================================
#define GATE_OPEN_ANGLE       90
#define GATE_CLOSED_ANGLE     0
#define VEHICLE_DETECT_CM     5     // Detection distance (5cm)
#define SENSOR_COOLDOWN_MS    5000  // Cooldown between entry/exit

// ===========================================
// BLYNK VIRTUAL PINS
// ===========================================
#define VPIN_AVAILABLE_SLOTS  V0    // Display: Available slots
#define VPIN_OCCUPIED_SLOTS   V1    // Display: Occupied slots
#define VPIN_TOTAL_SLOTS      V2    // Display: Total slots
#define VPIN_GATE_STATUS      V3    // Display: Gate status (Open/Closed)
#define VPIN_ENTRY_COUNT      V4    // Display: Total entries today
#define VPIN_EXIT_COUNT       V5    // Display: Total exits today
#define VPIN_GATE_BUTTON      V6    // Button: Manual gate control
#define VPIN_TERMINAL         V7    // Terminal: Log messages
#define VPIN_OCCUPANCY_RATE   V8    // Gauge: Occupancy percentage
#define VPIN_LAST_EVENT       V9    // Display: Last event

// ===========================================
// GLOBAL OBJECTS
// ===========================================
BlynkTimer timer;
Servo gateServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WidgetTerminal terminal(VPIN_TERMINAL);

// State variables
int occupiedSlots = 0;
int availableSlots = TOTAL_SLOTS;
bool gateOpen = false;
unsigned long gateOpenTime = 0;
int entryCount = 0;
int exitCount = 0;

// Sensor cooldown state
unsigned long entrySensorCooldownUntil = 0;
unsigned long exitSensorCooldownUntil = 0;

// ===========================================
// SETUP
// ===========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Smart Parking - Blynk ===");
  
  // I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // LEDs & Buzzer
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Ultrasonic Sensors
  pinMode(US_ENTRY_TRIG, OUTPUT);
  pinMode(US_ENTRY_ECHO, INPUT);
  pinMode(US_EXIT_TRIG, OUTPUT);
  pinMode(US_EXIT_ECHO, INPUT);
  Serial.println("[OK] Sensors initialized");
  
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Blynk Version");
  delay(1000);
  
  // Servo
  ESP32PWM::allocateTimer(1);
  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, 500, 2400);
  gateServo.write(GATE_CLOSED_ANGLE);
  Serial.println("[OK] Servo initialized");
  
  // Connect to WiFi & Blynk
  lcd.clear();
  lcd.print("Connecting...");
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  
  lcd.clear();
  lcd.print("WiFi OK!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  
  // Setup Blynk timer
  timer.setInterval(100L, checkSensors);       // Check sensors every 100ms
  timer.setInterval(1000L, updateBlynk);       // Update Blynk every 1 second
  timer.setInterval(500L, updateLCD);          // Update LCD every 500ms
  timer.setInterval(100L, checkAutoCloseGate); // Check gate auto-close
  
  // Initial Blynk sync
  updateBlynk();
  terminal.println("System started!");
  terminal.println("Ready for parking...");
  terminal.flush();
  
  Serial.println("\n=== READY ===");
}

// ===========================================
// LOOP
// ===========================================
void loop() {
  Blynk.run();
  timer.run();
  updateLEDs();
}

// ===========================================
// SENSOR FUNCTIONS
// ===========================================
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

void checkSensors() {
  unsigned long now = millis();
  
  // Check Entry sensor (only if not in cooldown)
  if (now > exitSensorCooldownUntil) {
    float entryDistance = measureDistance(US_ENTRY_TRIG, US_ENTRY_ECHO);
    if (entryDistance > 0 && entryDistance < VEHICLE_DETECT_CM && !gateOpen) {
      handleEntry();
      exitSensorCooldownUntil = now + SENSOR_COOLDOWN_MS;
    }
  }
  
  // Check Exit sensor (only if not in cooldown)
  if (now > entrySensorCooldownUntil) {
    float exitDistance = measureDistance(US_EXIT_TRIG, US_EXIT_ECHO);
    if (exitDistance > 0 && exitDistance < VEHICLE_DETECT_CM && !gateOpen) {
      handleExit();
      entrySensorCooldownUntil = now + SENSOR_COOLDOWN_MS;
    }
  }
}

// ===========================================
// ENTRY/EXIT HANDLERS
// ===========================================
void handleEntry() {
  Serial.println("[ENTRY] Vehicle detected!");
  
  if (availableSlots > 0) {
    openGate();
    occupiedSlots++;
    availableSlots = TOTAL_SLOTS - occupiedSlots;
    entryCount++;
    
    beep(1);
    terminal.println("🚗 ENTRY: Vehicle entered");
    terminal.flush();
    
    Blynk.virtualWrite(VPIN_LAST_EVENT, "Entry - " + String(entryCount));
  } else {
    // Parking full
    beep(3);
    terminal.println("❌ FULL: Entry denied");
    terminal.flush();
    
    lcd.clear();
    lcd.print("PARKIR PENUH!");
    delay(2000);
  }
  
  updateBlynk();
}

void handleExit() {
  Serial.println("[EXIT] Vehicle detected!");
  
  if (occupiedSlots > 0) {
    openGateForExit();
    occupiedSlots--;
    availableSlots = TOTAL_SLOTS - occupiedSlots;
    exitCount++;
    
    beep(1);
    terminal.println("🚙 EXIT: Vehicle exited");
    terminal.flush();
    
    Blynk.virtualWrite(VPIN_LAST_EVENT, "Exit - " + String(exitCount));
  }
  
  updateBlynk();
}

// ===========================================
// GATE CONTROL
// ===========================================
void openGate() {
  gateServo.write(GATE_OPEN_ANGLE);
  gateOpen = true;
  gateOpenTime = millis();
  
  lcd.clear();
  lcd.print("SELAMAT DATANG!");
  lcd.setCursor(0, 1);
  lcd.print("Silakan Masuk");
  
  Serial.println("[GATE] Opened for ENTRY");
  Blynk.virtualWrite(VPIN_GATE_STATUS, "OPEN");
}

void openGateForExit() {
  gateServo.write(GATE_OPEN_ANGLE);
  gateOpen = true;
  gateOpenTime = millis();
  
  lcd.clear();
  lcd.print("TERIMA KASIH!");
  lcd.setCursor(0, 1);
  lcd.print("Hati-hati...");
  
  Serial.println("[GATE] Opened for EXIT");
  Blynk.virtualWrite(VPIN_GATE_STATUS, "OPEN");
}

void closeGate() {
  gateServo.write(GATE_CLOSED_ANGLE);
  gateOpen = false;
  Serial.println("[GATE] Closed");
  Blynk.virtualWrite(VPIN_GATE_STATUS, "CLOSED");
}

void checkAutoCloseGate() {
  if (gateOpen && gateOpenTime > 0 && (millis() - gateOpenTime > 5000)) {
    closeGate();
    gateOpenTime = 0;
  }
}

// Manual gate control from Blynk app
BLYNK_WRITE(VPIN_GATE_BUTTON) {
  int value = param.asInt();
  if (value == 1) {
    if (gateOpen) {
      closeGate();
      terminal.println("📱 Gate closed manually");
    } else {
      openGate();
      terminal.println("📱 Gate opened manually");
    }
    terminal.flush();
  }
}

// ===========================================
// DISPLAY FUNCTIONS
// ===========================================
void updateLCD() {
  if (gateOpen) return;  // Don't update while showing gate message
  
  lcd.clear();
  lcd.print("PARKIR CERDAS");
  lcd.setCursor(0, 1);
  lcd.print("Slot: ");
  lcd.print(availableSlots);
  lcd.print("/");
  lcd.print(TOTAL_SLOTS);
  
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
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ===========================================
// BLYNK FUNCTIONS
// ===========================================
void updateBlynk() {
  Blynk.virtualWrite(VPIN_AVAILABLE_SLOTS, availableSlots);
  Blynk.virtualWrite(VPIN_OCCUPIED_SLOTS, occupiedSlots);
  Blynk.virtualWrite(VPIN_TOTAL_SLOTS, TOTAL_SLOTS);
  Blynk.virtualWrite(VPIN_ENTRY_COUNT, entryCount);
  Blynk.virtualWrite(VPIN_EXIT_COUNT, exitCount);
  
  // Calculate occupancy rate
  int occupancyRate = (occupiedSlots * 100) / TOTAL_SLOTS;
  Blynk.virtualWrite(VPIN_OCCUPANCY_RATE, occupancyRate);
  
  Blynk.virtualWrite(VPIN_GATE_STATUS, gateOpen ? "OPEN" : "CLOSED");
}

// Sync when connected to Blynk
BLYNK_CONNECTED() {
  Blynk.syncAll();
  terminal.println("✅ Connected to Blynk!");
  terminal.flush();
}
