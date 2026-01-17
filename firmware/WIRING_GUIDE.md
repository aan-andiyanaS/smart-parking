# ESP32-S3 Smart Parking - Wiring Guide

## 📋 Komponen yang Dibutuhkan

| No | Komponen | Qty | Keterangan |
|----|----------|-----|------------|
| 1 | ESP32-S3 WROOM N16R8 CAM | 1 | Dengan kamera OV2640 built-in |
| 2 | Servo Motor SG90/MG996R | 1 | Untuk gate barrier |
| 3 | LCD 16x2 I2C | 1 | Display status |
| 4 | VL53L0X ToF Sensor | 1 | Deteksi kendaraan |
| 5 | LED Hijau 5mm | 1 | Indikator tersedia |
| 6 | LED Merah 5mm | 1 | Indikator penuh |
| 7 | Buzzer Aktif 5V | 1 | Alarm |
| 8 | Resistor 220Ω | 2 | Untuk LED |
| 9 | Breadboard | 1 | Prototyping |
| 10 | Kabel Jumper | ~20 | Koneksi |
| 11 | Power Supply 5V 2A | 1 | Untuk servo |

---

## 📌 Wiring Diagram

```
                    ESP32-S3 WROOM N16R8
                    ┌──────────────────┐
                    │                  │
        ┌───────────┤ 3.3V         GND ├───────────┐
        │           │                  │           │
        │   ┌───────┤ GPIO 1 (SDA)     │           │
        │   │       │                  │           │
        │   │   ┌───┤ GPIO 2 (SCL)     │           │
        │   │   │   │                  │           │
        │   │   │   ├ GPIO 4 (LED_G)───┼──►[LED]──┤
        │   │   │   │                  │           │
        │   │   │   ├ GPIO 5 (LED_R)───┼──►[LED]──┤
        │   │   │   │                  │           │
        │   │   │   ├ GPIO 6 (BUZZER)──┼──►[BUZ]──┤
        │   │   │   │                  │           │
        │   │   │   ├ GPIO 21 (SERVO)──┼──►[SRV]  │
        │   │   │   │                  │           │
        │   │   │   │   [OV2640 CAM]   │           │
        │   │   │   │   (Built-in)     │           │
        │   │   │   │                  │           │
        │   │   │   └──────────────────┘           │
        │   │   │                                  │
        ▼   ▼   ▼                                  ▼
      ┌─────────────────────────────────────────────┐
      │                  I2C BUS                    │
      │  ┌─────────────┐     ┌─────────────┐       │
      │  │  LCD 16x2   │     │  VL53L0X    │       │
      │  │  I2C 0x27   │     │  I2C 0x29   │       │
      │  │             │     │             │       │
      │  │ SDA ◄───────┼─────┼──► SDA      │       │
      │  │ SCL ◄───────┼─────┼──► SCL      │       │
      │  │ VCC ◄───3.3V┼─────┼──► VCC      │       │
      │  │ GND ◄───GND─┼─────┼──► GND      │       │
      │  └─────────────┘     └─────────────┘       │
      └─────────────────────────────────────────────┘
```

---

## 📍 Pin Assignment

### ESP32-S3 GPIO Mapping

| GPIO | Fungsi | Koneksi |
|------|--------|---------|
| **GPIO 1** | I2C SDA | LCD SDA, VL53L0X SDA |
| **GPIO 2** | I2C SCL | LCD SCL, VL53L0X SCL |
| **GPIO 4** | LED Green | LED Hijau + Resistor 220Ω |
| **GPIO 5** | LED Red | LED Merah + Resistor 220Ω |
| **GPIO 6** | Buzzer | Buzzer Aktif |
| **GPIO 21** | Servo PWM | Servo Signal (Orange) |
| **3.3V** | Power | LCD VCC, VL53L0X VCC |
| **5V** | Power | Servo VCC (Merah) |
| **GND** | Ground | Semua GND komponen |

### Kamera OV2640 (Built-in)

| Pin | GPIO | Keterangan |
|-----|------|------------|
| XCLK | 10 | Clock |
| SIOD | 40 | I2C Data (Camera) |
| SIOC | 39 | I2C Clock (Camera) |
| Y2-Y9 | 15,17,18,16,14,12,11,48 | Data pins |
| VSYNC | 38 | Vertical Sync |
| HREF | 47 | Horizontal Ref |
| PCLK | 13 | Pixel Clock |

---

## 🔌 Detail Wiring

### 1. LCD 16x2 I2C

```
LCD I2C     →    ESP32-S3
────────────────────────────
VCC         →    3.3V
GND         →    GND
SDA         →    GPIO 1
SCL         →    GPIO 2
```

### 2. VL53L0X ToF Sensor

```
VL53L0X     →    ESP32-S3
────────────────────────────
VCC         →    3.3V
GND         →    GND
SDA         →    GPIO 1 (shared with LCD)
SCL         →    GPIO 2 (shared with LCD)
```

### 3. Servo Motor

```
Servo       →    ESP32-S3
────────────────────────────
Signal (Orange/Yellow)  →  GPIO 21
VCC (Red)              →  5V External
GND (Brown/Black)      →  GND
```

> ⚠️ **PENTING:** Servo membutuhkan power supply terpisah 5V 2A. Jangan langsung dari ESP32!

### 4. LED Hijau

```
GPIO 4 ──► [Resistor 220Ω] ──► [LED Hijau +] ──► [LED -] ──► GND
```

### 5. LED Merah

```
GPIO 5 ──► [Resistor 220Ω] ──► [LED Merah +] ──► [LED -] ──► GND
```

### 6. Buzzer

```
GPIO 6 ──► [Buzzer +] ──► [Buzzer -] ──► GND
```

---

## ⚡ Power Supply

```
┌─────────────────────────────────────────┐
│           Power Distribution            │
├─────────────────────────────────────────┤
│                                         │
│   [5V 2A Power Supply]                  │
│          │                              │
│          ├──► ESP32-S3 (via USB-C)      │
│          │                              │
│          └──► Servo VCC (Red wire)      │
│                                         │
│   ESP32-S3 3.3V ──► LCD, VL53L0X        │
│   ESP32-S3 GND  ──► All GND             │
│                                         │
└─────────────────────────────────────────┘
```

---

## 📸 Camera OV2640 Orientation

```
    ┌─────────────────────────┐
    │     ESP32-S3 Board      │
    │  ┌───────────────────┐  │
    │  │                   │  │
    │  │     [  O  ]       │  │  ◄── Camera Lens
    │  │    OV2640         │  │      (point toward parking area)
    │  │                   │  │
    │  └───────────────────┘  │
    │                         │
    │  [USB-C]  [Reset] [Boot]│
    └─────────────────────────┘
```

---

## ✅ Checklist Sebelum Upload

- [ ] WiFi SSID dan Password sudah diisi di `main.cpp`
- [ ] Server URL sudah diisi (IP address backend)
- [ ] Semua koneksi sudah dicek
- [ ] Power supply servo terpisah dari ESP32
- [ ] I2C address LCD dan VL53L0X benar (0x27 dan 0x29)

---

## 🚀 Upload Firmware

```bash
cd firmware

# Build
pio run

# Upload
pio run -t upload

# Monitor Serial
pio device monitor
```
