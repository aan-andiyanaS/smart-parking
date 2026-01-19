/*
 * Smart Parking MVP - Blynk Version
 * Versi Sederhana untuk Demo Cepat
 * 
 * Fitur:
 * - 2x Sensor Ultrasonic (Entry/Exit)
 * - Servo Gate
 * - LCD Display
 * - Monitoring via Blynk (4 widget)
 */

#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Parking"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== KONFIGURASI WiFi =====
const char* ssid = "ANS030005";
const char* pass = "71311311203";

// ===== PIN SENSOR =====
#define US_ENTRY_TRIG  1
#define US_ENTRY_ECHO  2
#define US_EXIT_TRIG   42
#define US_EXIT_ECHO   41
#define SERVO_PIN      14
#define BUZZER_PIN     7
#define I2C_SDA        21
#define I2C_SCL        20

// ===== BLYNK VIRTUAL PINS (4 widget) =====
#define V_AVAILABLE    V0   // Gauge: Slot tersedia
#define V_VEHICLE      V1   // Label: Status mobil (MASUK/KELUAR)
#define V_GATE_STATUS  V2   // Label: Status pintu (BUKA/TUTUP)
#define V_GATE_BTN     V3   // Button: Kontrol manual pintu

// ===== KONSTANTA =====
#define TOTAL_SLOTS        4
#define DETECT_DISTANCE    5     // cm
#define COOLDOWN_MS        5000  // 5 detik

// ===== OBJEK =====
Servo gate;
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

// ===== STATE =====
int occupied = 0;
int available = TOTAL_SLOTS;
bool gateOpen = false;
unsigned long gateTime = 0;
unsigned long entryCooldown = 0;
unsigned long exitCooldown = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Parking MVP ===");
  
  // I2C & LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("MVP v1.0");
  
  // Sensors & Buzzer
  pinMode(US_ENTRY_TRIG, OUTPUT);
  pinMode(US_ENTRY_ECHO, INPUT);
  pinMode(US_EXIT_TRIG, OUTPUT);
  pinMode(US_EXIT_ECHO, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Servo
  ESP32PWM::allocateTimer(1);
  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(0);
  
  // Blynk
  lcd.clear();
  lcd.print("Connecting...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  lcd.clear();
  lcd.print("Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  
  // Timer
  timer.setInterval(100L, checkSensors);
  timer.setInterval(100L, autoCloseGate);
  
  updateDisplay();
  Serial.println("READY!");
}

// ===== LOOP =====
void loop() {
  Blynk.run();
  timer.run();
}

// ===== SENSOR =====
float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long d = pulseIn(echo, HIGH, 30000);
  return d == 0 ? -1 : d * 0.034 / 2;
}

void checkSensors() {
  unsigned long now = millis();
  
  // Entry
  if (now > exitCooldown) {
    float d = getDistance(US_ENTRY_TRIG, US_ENTRY_ECHO);
    if (d > 0 && d < DETECT_DISTANCE && !gateOpen) {
      if (available > 0) {
        beep(1);  // Bip sekali
        Blynk.virtualWrite(V_VEHICLE, "🚗 Mobil MASUK");
        Blynk.logEvent("vehicle_entry", "Kendaraan masuk. Slot tersisa: " + String(available - 1));
        openGate("MASUK");
        occupied++;
        available = TOTAL_SLOTS - occupied;
        Blynk.virtualWrite(V_AVAILABLE, available);
      } else {
        beepFast(5);  // Bip cepat 5x - PENUH!
        lcd.clear();
        lcd.print("PARKIR PENUH!");
        Blynk.virtualWrite(V_VEHICLE, "❌ PENUH - Ditolak");
        Blynk.logEvent("parking_full", "Parkir PENUH! Kendaraan ditolak masuk.");
        delay(2000);
      }
      exitCooldown = now + COOLDOWN_MS;
      updateDisplay();
    }
  }
  
  // Exit
  if (now > entryCooldown) {
    float d = getDistance(US_EXIT_TRIG, US_EXIT_ECHO);
    if (d > 0 && d < DETECT_DISTANCE && !gateOpen && occupied > 0) {
      beep(1);  // Bip sekali
      Blynk.virtualWrite(V_VEHICLE, "🚙 Mobil KELUAR");
      Blynk.logEvent("vehicle_exit", "Kendaraan keluar. Slot tersedia: " + String(available + 1));
      openGate("KELUAR");
      occupied--;
      available = TOTAL_SLOTS - occupied;
      Blynk.virtualWrite(V_AVAILABLE, available);
      entryCooldown = now + COOLDOWN_MS;
      updateDisplay();
    }
  }
}

// ===== BUZZER =====
void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void beepFast(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
  }
}

void openGate(String tipe) {
  gate.write(90);
  gateOpen = true;
  gateTime = millis();
  
  // Update status pintu ke Blynk
  Blynk.virtualWrite(V_GATE_STATUS, "🟢 PINTU BUKA");
  
  lcd.clear();
  if (tipe == "MASUK") {
    lcd.print("SELAMAT DATANG!");
  } else if (tipe == "KELUAR") {
    lcd.print("TERIMA KASIH!");
  } else {
    lcd.print("PINTU TERBUKA");
  }
  Serial.println("[GATE] BUKA - " + tipe);
}

void autoCloseGate() {
  if (gateOpen && millis() - gateTime > 5000) {
    gate.write(0);
    gateOpen = false;
    
    // Update status pintu ke Blynk
    Blynk.virtualWrite(V_GATE_STATUS, "🔴 PINTU TUTUP");
    Serial.println("[GATE] TUTUP - Auto");
    
    updateDisplay();
  }
}

// Manual gate dari Blynk
BLYNK_WRITE(V_GATE_BTN) {
  if (param.asInt() == 1) {
    if (gateOpen) {
      gate.write(0);
      gateOpen = false;
      Blynk.virtualWrite(V_GATE_STATUS, "🔴 PINTU TUTUP");
      Blynk.virtualWrite(V_VEHICLE, "📱 Manual: Tutup");
      Serial.println("[GATE] TUTUP - Manual");
    } else {
      Blynk.virtualWrite(V_VEHICLE, "📱 Manual: Buka");
      openGate("MANUAL");
    }
  }
}

// ===== DISPLAY =====
void updateDisplay() {
  if (gateOpen) return;
  lcd.clear();
  lcd.print("SLOT TERSEDIA:");
  lcd.setCursor(0, 1);
  lcd.print(available);
  lcd.print(" / ");
  lcd.print(TOTAL_SLOTS);
  if (available == 0) lcd.print(" PENUH");
}

// Sync saat connect
BLYNK_CONNECTED() {
  Blynk.virtualWrite(V_AVAILABLE, available);
  Blynk.virtualWrite(V_VEHICLE, "✅ Online");
  Blynk.virtualWrite(V_GATE_STATUS, gateOpen ? "🟢 PINTU BUKA" : "🔴 PINTU TUTUP");
}
