# Smart Parking MVP - Blynk

**Versi paling sederhana untuk demo cepat (~2 jam setup)**

---

## 🎯 Fitur MVP

- ✅ 2 Sensor Ultrasonic (Entry/Exit)
- ✅ Servo Gate otomatis
- ✅ LCD Status
- ✅ **3 Widget Blynk saja**

---

## 🔌 Wiring Lengkap

### Diagram Koneksi

```
                        ESP32-S3 WROOM
                ┌─────────────────────────────────┐
                │          [USB-C]                │
                │                                 │
     GND ●──────┤ GND                        3.3V├──────● 3.3V
                │                                 │
                │                                 │
  LCD SDA ○─────┤ GPIO 21                 GPIO 20├─────○ LCD SCL
                │                                 │
  SERVO  ○──────┤ GPIO 14                        │
                │                                 │
 US1 TRIG ○─────┤ GPIO 1                   GPIO 2├─────○ US1 ECHO
                │                                 │
 US2 TRIG ○─────┤ GPIO 42                 GPIO 41├─────○ US2 ECHO
                │                                 │
     5V  ●──────┤ 5V (VIN)                       │
                │                                 │
                └─────────────────────────────────┘
```

### Detail Koneksi Per Komponen

#### 1️⃣ Sensor Ultrasonic ENTRY (HC-SR04 #1)

```
HC-SR04 #1          ESP32-S3
──────────          ────────
   VCC    ─────────►  5V
   TRIG   ─────────►  GPIO 1
   ECHO   ─────────►  GPIO 2
   GND    ─────────►  GND
```

#### 2️⃣ Sensor Ultrasonic EXIT (HC-SR04 #2)

```
HC-SR04 #2          ESP32-S3
──────────          ────────
   VCC    ─────────►  5V
   TRIG   ─────────►  GPIO 42
   ECHO   ─────────►  GPIO 41
   GND    ─────────►  GND
```

#### 3️⃣ Servo Motor (SG90)

```
Servo SG90          ESP32-S3
──────────          ────────
Brown (GND) ───────►  GND
Red (VCC)   ───────►  5V
Orange (SIG)───────►  GPIO 14
```

#### 4️⃣ LCD 16x2 I2C

```
LCD I2C             ESP32-S3
───────             ────────
   GND    ─────────►  GND
   VCC    ─────────►  5V
   SDA    ─────────►  GPIO 21
   SCL    ─────────►  GPIO 20
```

### Tabel Ringkasan Pin

| Komponen | Pin ESP32 | Warna Kabel (Saran) |
|----------|-----------|---------------------|
| **US Entry** | | |
| VCC | 5V | Merah |
| TRIG | GPIO 1 | Kuning |
| ECHO | GPIO 2 | Hijau |
| GND | GND | Hitam |
| **US Exit** | | |
| VCC | 5V | Merah |
| TRIG | GPIO 42 | Kuning |
| ECHO | GPIO 41 | Hijau |
| GND | GND | Hitam |
| **Servo** | | |
| VCC | 5V | Merah |
| Signal | GPIO 14 | Oranye |
| GND | GND | Coklat |
| **LCD I2C** | | |
| VCC | 5V | Merah |
| SDA | GPIO 21 | Biru |
| SCL | GPIO 20 | Putih |
| GND | GND | Hitam |

### Layout Fisik di Breadboard

```
     [ESP32-S3 WROOM]
           │
     ══════╪══════════════════════════════════
           │
    ┌──────┴──────┐
    │             │
    │  LCD 16x2   │
    │   I2C       │
    └─────────────┘
    
    
    ┌────────┐                    ┌────────┐
    │ US #1  │  ←── ENTRY         │ US #2  │  ←── EXIT
    │        │                    │        │
    └────────┘                    └────────┘
         │                             │
         │      ╔═══════════╗          │
         │      ║   GATE    ║          │
         └─────►║  (SERVO)  ║◄─────────┘
                ╚═══════════╝
```

---

## 📱 Setup Blynk (5 Menit)

### 1. Buat Template

Di https://blynk.cloud:
- **Name**: `SmartParkingMVP`
- **Hardware**: `ESP32`
- **Connection**: `WiFi`

### 2. Tambah Datastreams (3 saja!)

| Pin | Name | Type | Min | Max |
|-----|------|------|-----|-----|
| V0 | Available | Integer | 0 | 4 |
| V1 | Status | String | - | - |
| V2 | Gate | Integer | 0 | 1 |

### 3. Buat Dashboard

Tambahkan **3 widget saja**:

```
┌─────────────────────────────────────┐
│                                     │
│   ┌─────────────────────────────┐   │
│   │      SLOT TERSEDIA          │   │
│   │           2/4               │   │  ← Gauge (V0)
│   │         [====  ]            │   │
│   └─────────────────────────────┘   │
│                                     │
│   ┌─────────────────────────────┐   │
│   │  🚗 Mobil MASUK             │   │  ← Label (V1)
│   └─────────────────────────────┘   │
│                                     │
│   ┌─────────────────────────────┐   │
│   │       [ GATE ]              │   │  ← Button (V2)
│   └─────────────────────────────┘   │
│                                     │
└─────────────────────────────────────┘
```

### 4. Buat Device & Copy Token

1. **+ New Device** → From Template
2. Copy `BLYNK_TEMPLATE_ID` dan `BLYNK_AUTH_TOKEN`

### 5. Update Firmware

```cpp
#define BLYNK_TEMPLATE_ID "TMPLxxxxxx"     // Ganti!
#define BLYNK_TEMPLATE_NAME "SmartParkingMVP"
#define BLYNK_AUTH_TOKEN "xxxxxxxx"        // Ganti!

const char* ssid = "WiFi_Anda";            // Ganti!
const char* pass = "Password_WiFi";        // Ganti!
```

### 6. Upload & Done! 🎉

---

## 🔧 Library yang Diperlukan

1. **Blynk** by Volodymyr Shymanskyy
2. **ESP32Servo** by Kevin Harrington
3. **LiquidCrystal I2C** by Frank de Brabander

---

## ⚡ Cara Kerja Singkat

```
Sensor Entry aktif → Ada slot? → YA → Buka gate, slot -1
                              → TIDAK → LCD "PENUH!"

Sensor Exit aktif → Buka gate, slot +1

Cooldown 5 detik antar sensor (anti double-trigger)
Gate otomatis tutup setelah 5 detik
```

---

## 📋 Checklist Demo

- [ ] WiFi credentials sudah diganti
- [ ] Blynk token sudah diganti
- [ ] Semua sensor terhubung
- [ ] Upload firmware
- [ ] Buka Blynk app
- [ ] Test Entry (dekatkan objek ke sensor 1)
- [ ] Test Exit (dekatkan objek ke sensor 2)
- [ ] Test tombol Gate di app

---

## 🆘 Troubleshooting Cepat

| Masalah | Solusi |
|---------|--------|
| Tidak connect Blynk | Cek token & WiFi |
| LCD kosong | Coba I2C address `0x3F` |
| Servo tidak gerak | Cek power 5V |
| Sensor tidak respon | Cek wiring TRIG/ECHO |

---

**Waktu setup: ~30 menit**
**Waktu demo: ~10 menit**

Good luck! 🚀
