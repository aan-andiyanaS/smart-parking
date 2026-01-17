# Wiring Guide - Smart Parking ESP32-S3

## 📦 Komponen yang Dibutuhkan

| No | Komponen | Qty | Keterangan |
|----|----------|-----|------------|
| 1 | ESP32-S3 WROOM N16R8 + OV2640 | 1 | Camera module built-in |
| 2 | HC-SR04 Ultrasonic Sensor | 2 | Entry & Exit detection |
| 3 | SG90 Servo Motor | 1 | Gate control |
| 4 | LCD 16x2 I2C | 1 | Display status |
| 5 | LED Hijau 5mm | 1 | Indikator slot tersedia |
| 6 | LED Merah 5mm | 1 | Indikator penuh |
| 7 | Resistor 220Ω | 2 | Untuk LED |
| 8 | Breadboard | 1 | |
| 9 | Kabel Jumper | ~20 | Male-to-Male & Male-to-Female |

---

## 🔌 Wiring Diagram

```
                            ESP32-S3 WROOM N16R8
                    ┌─────────────────────────────────┐
                    │                                 │
        3.3V ───────┤ 3.3V                       GND ├─────── GND
                    │                                 │
                    │                                 │
   ┌── LCD SDA ─────┤ GPIO 21                GPIO 20 ├───── LCD SCL ──┐
   │                │                                 │               │
   │                │                                 │               │
   │   SERVO ───────┤ GPIO 14                        │               │
   │                │                                 │               │
   │   US1 TRIG ────┤ GPIO 1                  GPIO 2 ├───── US1 ECHO  │
   │                │                                 │               │
   │   US2 TRIG ────┤ GPIO 42                GPIO 41 ├───── US2 ECHO  │
   │                │                                 │               │
   │   LED GREEN ───┤ GPIO 4                  GPIO 5 ├───── LED RED   │
   │                │                                 │               │
   │                │                    [OV2640 CAM] │               │
   │                │                    (Built-in)   │               │
   │                └─────────────────────────────────┘               │
   │                                                                   │
   │                                                                   │
   │              ┌──────────────────────────────────────────────────┘
   │              │
   ▼              ▼
┌──────────────────────┐
│   LCD 16x2 I2C       │
│  ┌────┬────┬────┬────┐
│  │GND │VCC │SDA │SCL │
│  └────┴────┴────┴────┘
│    │    │    │    │
│   GND  5V  G21  G20
└──────────────────────┘
```

---

## 📍 Pin Assignment Table

### ESP32-S3 WROOM N16R8 Pinout

| Komponen | Pin ESP32 | Keterangan |
|----------|-----------|------------|
| **LCD I2C** | | |
| SDA | GPIO 21 | Data I2C |
| SCL | GPIO 20 | Clock I2C |
| VCC | 5V | Power |
| GND | GND | Ground |
| **Servo (Gate)** | | |
| Signal (Orange) | GPIO 14 | PWM Signal |
| VCC (Red) | 5V | Power |
| GND (Brown) | GND | Ground |
| **Ultrasonic 1 (ENTRY)** | | |
| VCC | 5V | Power |
| GND | GND | Ground |
| TRIG | GPIO 1 | Trigger |
| ECHO | GPIO 2 | Echo |
| **Ultrasonic 2 (EXIT)** | | |
| VCC | 5V | Power |
| GND | GND | Ground |
| TRIG | GPIO 42 | Trigger |
| ECHO | GPIO 41 | Echo |
| **LED Green** | | |
| Anode (+) | GPIO 4 | Via 220Ω resistor |
| Cathode (-) | GND | |
| **LED Red** | | |
| Anode (+) | GPIO 5 | Via 220Ω resistor |
| Cathode (-) | GND | |

---

## 🔧 Wiring Per Komponen

### 1. LCD 16x2 I2C

```
LCD I2C          ESP32-S3
─────────        ────────
GND      ──────► GND
VCC      ──────► 5V (VIN)
SDA      ──────► GPIO 21
SCL      ──────► GPIO 20
```

> 📝 LCD I2C biasanya menggunakan alamat `0x27` atau `0x3F`. Jika LCD tidak muncul, coba ganti alamat di code.

### 2. Servo Motor SG90 (Gate)

```
Servo SG90       ESP32-S3
───────────      ────────
Brown (GND) ───► GND
Red (VCC)   ───► 5V (VIN)
Orange (Sig)───► GPIO 14
```

> ⚠️ Jika servo tidak stabil, gunakan power supply eksternal 5V untuk servo.

### 3. Ultrasonic HC-SR04 #1 (ENTRY Gate)

```
HC-SR04 #1       ESP32-S3
───────────      ────────
VCC        ────► 5V
GND        ────► GND
TRIG       ────► GPIO 1
ECHO       ────► GPIO 2
```

### 4. Ultrasonic HC-SR04 #2 (EXIT Gate)

```
HC-SR04 #2       ESP32-S3
───────────      ────────
VCC        ────► 5V
GND        ────► GND
TRIG       ────► GPIO 42
ECHO       ────► GPIO 41
```

### 5. LED Indicators

```
LED Green        ESP32-S3
─────────        ────────
Anode (+) ──[220Ω]──► GPIO 4
Cathode(-)──────────► GND

LED Red          ESP32-S3
───────          ────────
Anode (+) ──[220Ω]──► GPIO 5
Cathode(-)──────────► GND
```

---

## 🎨 Physical Layout (Top View)

```
                    ┌─────────────────────────────────────────┐
                    │           PARKING LOT                   │
                    │                                         │
                    │    ┌─────┐ ┌─────┐                     │
                    │    │ P1  │ │ P2  │                     │
                    │    └─────┘ └─────┘                     │
                    │    ┌─────┐ ┌─────┐                     │
                    │    │ P3  │ │ P4  │                     │
                    │    └─────┘ └─────┘                     │
                    │                                         │
                    │                📷                       │
                    │            [CAMERA]                     │
                    │                                         │
                    │    ┌────────────────────────┐          │
                    │    │      LCD 16x2          │          │
                    │    │  "PARKIR CERDAS"       │          │
                    │    │   Slot: 3/4            │          │
                    │    └────────────────────────┘          │
                    │                                         │
                    │   🟢    🔴                              │
                    │  [LED] [LED]                            │
                    │                                         │
    ════════════════╬═════════════════════════╬══════════════
         ENTRY      │                         │      EXIT
                    │                         │
    ┌────────┐      │      ╔═══════╗          │      ┌────────┐
    │ US #1  │      │      ║ GATE  ║          │      │ US #2  │
    │(HC-SR04)│     │      ║(SERVO)║          │      │(HC-SR04)│
    └────────┘      │      ╚═══════╝          │      └────────┘
                    │                         │
    ════════════════╬═════════════════════════╬══════════════
```

---

## ⚡ Power Requirements

| Komponen | Tegangan | Arus |
|----------|----------|------|
| ESP32-S3 | 3.3V/5V | ~240mA |
| LCD I2C | 5V | ~20mA |
| Servo SG90 | 5V | ~200-600mA |
| HC-SR04 x2 | 5V | ~30mA |
| LED x2 | 3.3V | ~20mA |
| **Total** | | **~600mA** |

> 💡 **Rekomendasi:** Gunakan power supply 5V 2A untuk menjamin stabilitas, terutama saat servo aktif.

---

## ⚠️ Troubleshooting

### LCD tidak muncul
- Cek alamat I2C: Coba `0x27` atau `0x3F`
- Cek wiring SDA/SCL tidak tertukar
- Putar potensiometer kontras di belakang LCD

### Servo tidak bergerak/gemetar
- Gunakan power supply eksternal 5V
- Pastikan GND servo terhubung ke GND ESP32

### Ultrasonic tidak akurat
- Jauhkan dari objek penghalang
- Pastikan permukaan target flat
- Hindari interference dari ultrasonic lain (beri jarak >2cm antar pengukuran)

### Camera tidak capture
- Cek pin camera sesuai dengan module
- Restart ESP32 jika camera freeze

---

## 📋 Checklist Sebelum Upload Code

- [ ] Semua komponen terhubung sesuai diagram
- [ ] Power supply memadai (5V 2A)
- [ ] WiFi SSID & Password sudah diganti di code
- [ ] Server URL sudah diganti di code
- [ ] I2C address LCD sudah benar (0x27 atau 0x3F)
