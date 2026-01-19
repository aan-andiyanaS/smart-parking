# Arduino IDE Firmware

Folder ini berisi firmware untuk Arduino IDE.

## 📁 Struktur Folder

```
arduino/
├── SmartParkingCam/      ← Camera + LCD only (SIMPLE)
│   └── SmartParkingCam.ino
├── SmartParkingFull/     ← Full version (ALL COMPONENTS)
│   └── SmartParkingFull.ino
└── README.md
```

---

## 🔵 SmartParkingCam (Camera Only)

**Komponen:**
- ✅ ESP32-S3 dengan OV2640 Camera
- ✅ LCD 16x2 I2C
- ✅ LED Indikator

**Fitur:**
- MJPEG live streaming ke dashboard
- YOLO detection via AI Service
- Status slot di LCD

**Gunakan ini jika:** Hanya punya kamera dan LCD, tanpa servo dan sensor.

---

## 🟢 SmartParkingFull (Full Version)

**Komponen:**
- ✅ ESP32-S3 dengan OV2640 Camera
- ✅ LCD 16x2 I2C
- ✅ Servo Motor (Gate)
- ✅ VL53L0X ToF Sensor (Deteksi Kendaraan)
- ✅ LED + Buzzer

**Fitur:**
- Semua fitur Camera Only
- Gate otomatis buka/tutup
- Deteksi kendaraan di gate
- Buzzer notifikasi
- **Session Tracking** - Kirim event entry/exit ke backend

**Gunakan ini jika:** Punya semua komponen lengkap.

---

## 🔧 Setup Arduino IDE

### 1. Install Board ESP32
- File → Preferences
- Tambah URL:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Tools → Board → Boards Manager → Install "esp32"

### 2. Install Library

| Library | Untuk |
|---------|-------|
| ArduinoJson | JSON parsing |
| LiquidCrystal I2C | LCD display |
| ESP32Servo | Servo motor (Full only) |
| VL53L0X | ToF sensor (Full only) |

### 3. Board Settings

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| PSRAM | OPI PSRAM |
| Flash Size | 16MB |
| Partition | Huge APP (3MB No OTA) |

### 4. Upload
1. Edit WiFi credentials di kode
2. Tekan tombol **BOOT** di ESP32
3. Klik **Upload** (→)
4. Lepas tombol setelah "Connecting..."

---

## 🌐 Endpoints HTTP

Setelah upload, buka browser ke `http://<ESP32_IP>/`:

| URL | Fungsi |
|-----|--------|
| `/` | Status page + live stream |
| `/stream` | MJPEG video stream |
| `/capture` | Single frame |
| `/status` | JSON status |
| `/restart` | Remote restart |
| `/gate/open` | Buka gate (Full only) |
| `/gate/close` | Tutup gate (Full only) |

---

## 📊 Session Tracking (Full Version)

Firmware Full Version akan otomatis mengirim event ke Backend saat kendaraan terdeteksi:

| Event | Endpoint Backend | Trigger |
|-------|------------------|---------|
| Entry | `POST /api/sessions/entry` | Sensor ToF deteksi kendaraan masuk |
| Exit | `POST /api/sessions/exit` | Sensor ultrasonic deteksi kendaraan keluar |

**Payload yang dikirim:**
```json
{
  "camera_id": "esp32-main",
  "event_type": "entry"  // atau "exit"
}
```

> ⚠️ **Catatan:** Backend perlu menyediakan endpoint `/api/sessions/entry` dan `/api/sessions/exit` untuk menerima event ini.

