# Smart Parking - Blynk Version

Firmware untuk Smart Parking System tanpa kamera, menggunakan **Blynk IoT** untuk monitoring real-time via smartphone.

---

## 📦 Komponen yang Dibutuhkan

| No | Komponen | Qty | Keterangan |
|----|----------|-----|------------|
| 1 | ESP32-S3 WROOM N16R8 | 1 | Tanpa kamera |
| 2 | HC-SR04 Ultrasonic Sensor | 2 | Entry & Exit detection |
| 3 | SG90 Servo Motor | 1 | Gate control |
| 4 | LCD 16x2 I2C | 1 | Display status |
| 5 | LED Hijau 5mm | 1 | Indikator slot tersedia |
| 6 | LED Merah 5mm | 1 | Indikator penuh |
| 7 | Buzzer 5V | 1 | Notifikasi suara |
| 8 | Resistor 220Ω | 2 | Untuk LED |
| 9 | Breadboard & Kabel Jumper | - | |

---

## 🔌 Wiring Diagram

```
                        ESP32-S3 WROOM N16R8
                ┌─────────────────────────────────┐
                │                                 │
    3.3V ───────┤ 3.3V                       GND ├─────── GND
                │                                 │
                │                                 │
 LCD SDA ───────┤ GPIO 21                GPIO 20 ├───── LCD SCL
                │                                 │
  SERVO ────────┤ GPIO 14                        │
                │                                 │
 US1 TRIG ──────┤ GPIO 1                  GPIO 2 ├───── US1 ECHO (ENTRY)
                │                                 │
 US2 TRIG ──────┤ GPIO 42                GPIO 41 ├───── US2 ECHO (EXIT)
                │                                 │
 LED GREEN ─────┤ GPIO 4                  GPIO 5 ├───── LED RED
                │                                 │
 BUZZER ────────┤ GPIO 7                         │
                │                                 │
                └─────────────────────────────────┘
```

---

## 📍 Pin Assignment

| Komponen | Pin ESP32 | Keterangan |
|----------|-----------|------------|
| **LCD I2C** | | |
| SDA | GPIO 21 | Data I2C |
| SCL | GPIO 20 | Clock I2C |
| VCC | 5V | Power |
| GND | GND | Ground |
| **Servo (Gate)** | | |
| Signal | GPIO 14 | PWM Signal |
| VCC | 5V | Power |
| GND | GND | Ground |
| **Ultrasonic 1 (ENTRY)** | | |
| TRIG | GPIO 1 | Trigger |
| ECHO | GPIO 2 | Echo |
| VCC | 5V | Power |
| GND | GND | Ground |
| **Ultrasonic 2 (EXIT)** | | |
| TRIG | GPIO 42 | Trigger |
| ECHO | GPIO 41 | Echo |
| VCC | 5V | Power |
| GND | GND | Ground |
| **LED Green** | GPIO 4 | Via 220Ω resistor |
| **LED Red** | GPIO 5 | Via 220Ω resistor |
| **Buzzer** | GPIO 7 | Active buzzer 5V |

---

## 📱 Setup Blynk

### Step 1: Buat Akun Blynk

1. Download **Blynk IoT** app dari Play Store / App Store
2. Buat akun di https://blynk.cloud
3. Login ke Blynk Console

### Step 2: Buat Template

1. Di Blynk Console, klik **+ New Template**
2. Isi:
   - **Name**: `Smart Parking`
   - **Hardware**: `ESP32`
   - **Connection Type**: `WiFi`
3. Klik **Done**

### Step 3: Setup Datastreams

Tambahkan **Virtual Pins** berikut di tab **Datastreams**:

| Virtual Pin | Name | Data Type | Min | Max |
|-------------|------|-----------|-----|-----|
| V0 | Available Slots | Integer | 0 | 10 |
| V1 | Occupied Slots | Integer | 0 | 10 |
| V2 | Total Slots | Integer | 0 | 10 |
| V3 | Gate Status | String | - | - |
| V4 | Entry Count | Integer | 0 | 999 |
| V5 | Exit Count | Integer | 0 | 999 |
| V6 | Gate Button | Integer | 0 | 1 |
| V7 | Terminal | String | - | - |
| V8 | Occupancy Rate | Integer | 0 | 100 |
| V9 | Last Event | String | - | - |

### Step 4: Buat Dashboard (Web)

Di tab **Web Dashboard**, tambahkan widgets:

| Widget | Datastream | Keterangan |
|--------|------------|------------|
| **Label** | V0 | Available Slots |
| **Label** | V1 | Occupied Slots |
| **Gauge** | V8 | Occupancy Rate (0-100%) |
| **Label** | V3 | Gate Status |
| **Button** | V6 | Manual Gate Control |
| **Terminal** | V7 | Event Log |
| **Label** | V4 | Total Entries |
| **Label** | V5 | Total Exits |
| **Label** | V9 | Last Event |

### Step 5: Buat Device

1. Klik **+ New Device**
2. Pilih **From Template** → **Smart Parking**
3. Beri nama device, klik **Create**
4. **COPY** kode berikut yang muncul:
   - `BLYNK_TEMPLATE_ID`
   - `BLYNK_TEMPLATE_NAME`
   - `BLYNK_AUTH_TOKEN`

### Step 6: Update Firmware

Buka `SmartParkingBlynk.ino` dan ganti:

```cpp
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"    // Dari Step 5
#define BLYNK_TEMPLATE_NAME "Smart Parking"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"      // Dari Step 5

const char* WIFI_SSID = "YOUR_WIFI_SSID";       // WiFi Anda
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

### Step 7: Setup Mobile Dashboard

1. Buka Blynk IoT app di HP
2. Pilih device **Smart Parking**
3. Klik ikon **🔧** untuk edit dashboard
4. Tambahkan widgets sama seperti Web Dashboard
5. Save

---

## 🔄 Cara Kerja Sistem

### Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           HARDWARE LAYER                                │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────┐  ┌─────────┐  ┌─────┐ │
│  │ US Entry     │  │ US Exit      │  │ Servo   │  │ LCD I2C │  │ LED │ │
│  │ (HC-SR04)    │  │ (HC-SR04)    │  │ (SG90)  │  │ (16x2)  │  │     │ │
│  └──────┬───────┘  └──────┬───────┘  └────┬────┘  └────┬────┘  └──┬──┘ │
│         │                 │               │            │          │    │
│         └─────────────────┴───────────────┴────────────┴──────────┘    │
│                                   │                                     │
│                                   ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    ESP32-S3 (Microcontroller)                   │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │   │
│  │  │ Sensor Task │  │ Gate Task   │  │ Blynk Communication     │  │   │
│  │  │ (100ms loop)│  │ (100ms loop)│  │ (1000ms sync)           │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                   │                                     │
└───────────────────────────────────┼─────────────────────────────────────┘
                                    │ WiFi
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           CLOUD LAYER                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      BLYNK CLOUD SERVER                         │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │   │
│  │  │ Datastreams │  │ Event Log   │  │ Device Management       │  │   │
│  │  │ V0-V9       │  │ (Terminal)  │  │ (Online/Offline)        │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                   │                                     │
└───────────────────────────────────┼─────────────────────────────────────┘
                                    │ Internet
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                                │
│  ┌─────────────────────┐              ┌─────────────────────┐          │
│  │   📱 Blynk Mobile   │              │   💻 Blynk Web      │          │
│  │   (iOS/Android)     │              │   (Console)         │          │
│  └─────────────────────┘              └─────────────────────┘          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

### Detail Proses Entry (Kendaraan Masuk)

```
 Waktu    Aksi
  0ms     Sensor Entry mendeteksi objek < 5cm
          │
  1ms     ├─► Cek cooldown: exitSensorCooldownUntil sudah lewat?
          │   └─► TIDAK: Abaikan (sensor dalam cooldown)
          │   └─► YA: Lanjut proses
          │
  2ms     ├─► Cek availableSlots > 0?
          │   │
          │   ├─► YA (ada slot):
          │   │   ├─► openGate() → Servo ke 90°
          │   │   ├─► occupiedSlots++ (1 → 2)
          │   │   ├─► availableSlots = TOTAL - occupied (4-2=2)
          │   │   ├─► entryCount++ (0 → 1)
          │   │   ├─► beep(1) → Buzzer 1x
          │   │   ├─► LCD: "SELAMAT DATANG!"
          │   │   ├─► Blynk.virtualWrite(V0, availableSlots)
          │   │   ├─► Blynk.virtualWrite(V1, occupiedSlots)
          │   │   ├─► Blynk.virtualWrite(V4, entryCount)
          │   │   └─► Terminal: "🚗 ENTRY: Vehicle entered"
          │   │
          │   └─► TIDAK (parkir penuh):
          │       ├─► beep(3) → Buzzer 3x (warning)
          │       ├─► LCD: "PARKIR PENUH!"
          │       └─► Terminal: "❌ FULL: Entry denied"
          │
  3ms     └─► Set cooldown: exitSensorCooldownUntil = now + 5000ms
              (Sensor exit dinonaktifkan selama 5 detik)
          
 5000ms   Gate otomatis tertutup (checkAutoCloseGate)
          └─► Servo ke 0°
```

---

### Detail Proses Exit (Kendaraan Keluar)

```
 Waktu    Aksi
  0ms     Sensor Exit mendeteksi objek < 5cm
          │
  1ms     ├─► Cek cooldown: entrySensorCooldownUntil sudah lewat?
          │   └─► TIDAK: Abaikan
          │   └─► YA: Lanjut proses
          │
  2ms     ├─► Cek occupiedSlots > 0? (ada kendaraan di dalam?)
          │   │
          │   └─► YA:
          │       ├─► openGateForExit() → Servo ke 90°
          │       ├─► occupiedSlots-- (2 → 1)
          │       ├─► availableSlots = TOTAL - occupied (4-1=3)
          │       ├─► exitCount++ (0 → 1)
          │       ├─► beep(1) → Buzzer 1x
          │       ├─► LCD: "TERIMA KASIH!"
          │       └─► Update Blynk (V0, V1, V5)
          │
  3ms     └─► Set cooldown: entrySensorCooldownUntil = now + 5000ms
```

---

### Mekanisme Sensor Cooldown

Cooldown digunakan untuk menghindari **double-trigger** ketika kedua sensor dipasang berdekatan (single gate):

```
Skenario TANPA Cooldown (MASALAH):
──────────────────────────────────
Kendaraan masuk → Sensor Entry aktif → Entry dicatat
                  Kendaraan masih bergerak...
                  Sensor Exit juga aktif → Exit dicatat (SALAH!)
                  
Hasil: Entry +1, Exit +1 (padahal baru masuk)


Skenario DENGAN Cooldown (BENAR):
─────────────────────────────────
Kendaraan masuk → Sensor Entry aktif → Entry dicatat
                  exitSensorCooldownUntil = now + 5000
                  │
                  │ Dalam 5 detik:
                  │ Sensor Exit aktif → DIABAIKAN (dalam cooldown)
                  │
                  ▼ Setelah 5 detik:
                  Sensor Exit siap digunakan lagi
                  
Hasil: Entry +1 saja (benar!)
```

**Nilai Cooldown**: 5000ms (5 detik) - cukup untuk kendaraan melewati gate

---

### State Variables (Variabel Status)

```cpp
// Tracking slot parkir
int occupiedSlots = 0;     // Jumlah slot terisi (0-4)
int availableSlots = 4;    // Jumlah slot tersedia (TOTAL - occupied)
int TOTAL_SLOTS = 4;       // Kapasitas maksimum

// Tracking gate
bool gateOpen = false;     // Status gate (true = terbuka)
unsigned long gateOpenTime; // Waktu gate dibuka (untuk auto-close)

// Counter harian
int entryCount = 0;        // Total kendaraan masuk hari ini
int exitCount = 0;         // Total kendaraan keluar hari ini

// Cooldown timestamp
unsigned long entrySensorCooldownUntil = 0;  // Entry sensor aktif setelah waktu ini
unsigned long exitSensorCooldownUntil = 0;   // Exit sensor aktif setelah waktu ini
```

---

### Timing & Interval

| Task | Interval | Fungsi |
|------|----------|--------|
| `checkSensors()` | 100ms | Baca sensor ultrasonic |
| `updateBlynk()` | 1000ms | Sync data ke Blynk Cloud |
| `updateLCD()` | 500ms | Refresh tampilan LCD |
| `checkAutoCloseGate()` | 100ms | Cek apakah gate perlu ditutup |
| Gate auto-close | 5000ms | Durasi gate terbuka sebelum otomatis tutup |
| Sensor cooldown | 5000ms | Jeda antar aktivasi sensor |

---

### Komunikasi dengan Blynk

**Data yang dikirim ESP32 → Blynk Cloud:**

```cpp
Blynk.virtualWrite(V0, availableSlots);  // Slot tersedia
Blynk.virtualWrite(V1, occupiedSlots);   // Slot terisi
Blynk.virtualWrite(V2, TOTAL_SLOTS);     // Total kapasitas
Blynk.virtualWrite(V3, "OPEN/CLOSED");   // Status gate
Blynk.virtualWrite(V4, entryCount);      // Counter entry
Blynk.virtualWrite(V5, exitCount);       // Counter exit
Blynk.virtualWrite(V8, occupancyRate);   // Persentase terisi (0-100%)
Blynk.virtualWrite(V9, "Entry - 5");     // Event terakhir
terminal.println("🚗 ENTRY: ...");       // Log ke terminal (V7)
```

**Data yang diterima Blynk → ESP32:**

```cpp
BLYNK_WRITE(V6) {  // Tombol gate di app ditekan
  int value = param.asInt();
  if (value == 1) {
    // Toggle gate buka/tutup
  }
}
```

---

### Perhitungan Occupancy Rate

```cpp
int occupancyRate = (occupiedSlots * 100) / TOTAL_SLOTS;

// Contoh:
// occupiedSlots = 3, TOTAL_SLOTS = 4
// occupancyRate = (3 * 100) / 4 = 75%
```

Ditampilkan sebagai **Gauge** di Blynk app (0-100%)

---

## 🛠️ Upload Firmware

### Library yang Diperlukan

Install via Arduino IDE Library Manager:

1. **Blynk** by Volodymyr Shymanskyy
2. **ESP32Servo** by Kevin Harrington
3. **LiquidCrystal I2C** by Frank de Brabander

### Board Settings

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| PSRAM | OPI PSRAM |
| Flash Size | 16MB |
| Partition | Huge APP (3MB No OTA) |

### Upload

1. Hubungkan ESP32 via USB
2. Pilih **Board** dan **Port** yang sesuai
3. Klik **Upload** (→)
4. Tunggu sampai selesai

---

## 🧪 Testing

### Serial Monitor

Buka Serial Monitor (115200 baud) untuk melihat log:

```
=== Smart Parking - Blynk ===
[OK] Sensors initialized
[OK] Servo initialized
WiFi OK!
✅ Connected to Blynk!
=== READY ===
```

### Test Entry

1. Dekatkan tangan/objek ke sensor **Entry** (< 5cm)
2. Seharusnya:
   - Gate terbuka (servo 90°)
   - LCD: "SELAMAT DATANG!"
   - Buzzer berbunyi 1x
   - Blynk app update: Available slots -1

### Test Exit

1. Dekatkan tangan/objek ke sensor **Exit** (< 5cm)
2. Seharusnya:
   - Gate terbuka (servo 90°)
   - LCD: "TERIMA KASIH!"
   - Buzzer berbunyi 1x
   - Blynk app update: Available slots +1

### Test Manual Gate (Blynk)

1. Buka Blynk app
2. Tekan tombol **Gate**
3. Gate akan toggle buka/tutup

---

## ❓ Troubleshooting

### Tidak connect ke Blynk

- Pastikan `BLYNK_AUTH_TOKEN` benar
- Pastikan WiFi terhubung
- Cek firewall tidak block port Blynk

### Sensor tidak akurat

- Pastikan jarak objek < 5cm untuk trigger
- Hindari permukaan miring/tidak rata
- Cek wiring TRIG/ECHO tidak tertukar

### LCD tidak muncul

- Cek alamat I2C: coba `0x27` atau `0x3F`
- Putar potensiometer kontras di belakang LCD

### Servo tidak bergerak

- Gunakan power supply eksternal 5V
- Pastikan GND servo terhubung ke GND ESP32

---

## 📁 Struktur Folder

```
firmware/arduino/SmartParkingBlynk/
├── SmartParkingBlynk.ino   ← Main firmware
└── README.md               ← This file
```
