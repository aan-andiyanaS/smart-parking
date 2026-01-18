# PlatformIO IDE Setup Guide

Panduan lengkap setup PlatformIO IDE untuk upload firmware ke ESP32-S3.

---

## 📋 Persyaratan

- **Visual Studio Code** (VS Code)
- **USB driver** untuk ESP32 (biasanya otomatis di Windows 10/11)
- **Kabel USB-C** yang support data transfer (bukan charging only)

---

## 🔧 Langkah 1: Install VS Code

1. Download VS Code dari [code.visualstudio.com](https://code.visualstudio.com/)
2. Install dengan default settings
3. Buka VS Code setelah selesai install

---

## 📦 Langkah 2: Install PlatformIO Extension

1. Buka VS Code
2. Klik icon **Extensions** di sidebar kiri (Ctrl+Shift+X)
3. Ketik `PlatformIO IDE` di search box
4. Klik **Install** pada "PlatformIO IDE" by PlatformIO
5. **Tunggu** sampai instalasi selesai (bisa 2-5 menit)
6. **Restart VS Code** setelah selesai

> ⚠️ **Penting**: Pastikan koneksi internet stabil, PlatformIO akan download dependencies

---

## 📂 Langkah 3: Buka Project Firmware

1. Klik **File** → **Open Folder**
2. Navigate ke folder `smart-parking/firmware`
3. Klik **Select Folder**
4. Tunggu PlatformIO mendeteksi project (lihat loading di status bar bawah)

---

## ⚙️ Langkah 4: Install Platform ESP32

PlatformIO akan otomatis download platform saat pertama kali, tapi jika gagal:

1. Klik icon **PlatformIO** di sidebar kiri (icon seperti alien/semut)
2. Di panel PIO Home, klik **Platforms**
3. Search `Espressif 32`
4. Klik **Install**

---

## 🔌 Langkah 5: Hubungkan ESP32

1. Colok kabel USB-C ke ESP32-S3
2. Colok ujung lainnya ke laptop
3. Cek Windows Device Manager:
   - Buka **Device Manager** (Win + X → Device Manager)
   - Lihat di **Ports (COM & LPT)**
   - Harus ada device baru seperti `USB-SERIAL CH340 (COM3)`
   - Catat nomor COM-nya

> 💡 **Jika tidak muncul**: Install driver CH340 dari [sparks.gogo.co.nz](http://sparks.gogo.co.nz/ch340.html)

---

## ✏️ Langkah 6: Edit Konfigurasi WiFi

1. Buka file `src/main.cpp` atau `src/main_camera_only.cpp`
2. Edit bagian konfigurasi:

```cpp
const char* WIFI_SSID = "NamaWiFiAnda";
const char* WIFI_PASSWORD = "PasswordWiFiAnda";
const char* BACKEND_URL = "http://192.168.1.100:8080";  // IP laptop
const char* AI_SERVICE_URL = "http://192.168.1.100:5000";
```

3. **Cara cari IP laptop**:
   - Buka PowerShell
   - Ketik `ipconfig`
   - Lihat **IPv4 Address** di WiFi adapter

---

## 🔨 Langkah 7: Build/Compile Firmware

### Cara 1: Via Toolbar
1. Klik icon **✓ (centang)** di toolbar bawah VS Code
2. Tunggu sampai selesai (pertama kali bisa 2-3 menit)

### Cara 2: Via PlatformIO Menu
1. Klik icon **PlatformIO** di sidebar
2. Expand **PROJECT TASKS** → **esp32s3**
3. Klik **Build**

### Hasil yang Diharapkan:
```
========================= [SUCCESS] Took 45.32 seconds =========================
```

---

## 📤 Langkah 8: Upload Firmware ke ESP32

### Persiapan (PENTING!)
1. Tekan dan **TAHAN** tombol **BOOT** di ESP32
2. Sambil menahan BOOT, tekan tombol **EN/RST** sekali
3. **Lepas** tombol EN
4. **Lepas** tombol BOOT
5. ESP32 sekarang dalam mode upload

### Upload
1. Klik icon **→ (panah kanan)** di toolbar bawah VS Code
2. Atau: PlatformIO sidebar → **Upload**
3. Tunggu sampai selesai

### Hasil yang Diharapkan:
```
Writing at 0x00010000... (100 %)
Wrote 1234567 bytes in 45.6 seconds
Hard resetting via RTS pin...
========================= [SUCCESS] Took 52.18 seconds =========================
```

---

## 🖥️ Langkah 9: Monitor Serial (Opsional)

1. Klik icon **🔌 (colokan)** di toolbar bawah
2. Atau: PlatformIO sidebar → **Monitor**
3. Akan muncul output dari ESP32:

```
[OK] Camera initialized
[WIFI] Connecting to MyWiFi...
[OK] WiFi connected!
IP: 192.168.1.50
Stream URL: http://192.168.1.50/stream
```

---

## ❌ Troubleshooting

### Error: "No module named 'intelhex'"
```powershell
pip install intelhex
```

### Error: "Failed to connect to ESP32"
1. Pastikan sudah tekan tombol BOOT sebelum upload
2. Coba ganti kabel USB (harus support data, bukan charging only)
3. Cek Device Manager apakah port muncul

### Error: "Access Denied" pada COM port
1. Tutup Serial Monitor jika sedang terbuka
2. Tutup aplikasi lain yang mungkin menggunakan port (Arduino IDE, Putty, dll)

### Error: "Camera init failed"
1. Pastikan menggunakan board ESP32-S3 dengan camera OV2640
2. Cek pin definition di kode sesuai dengan board Anda

---

## 📝 Quick Reference

| Aksi | Shortcut/Button |
|------|-----------------|
| Build | Toolbar: ✓ atau Ctrl+Alt+B |
| Upload | Toolbar: → atau Ctrl+Alt+U |
| Serial Monitor | Toolbar: 🔌 atau Ctrl+Alt+S |
| Clean | PlatformIO sidebar → Clean |

---

## ✅ Checklist Sebelum Upload

- [ ] VS Code + PlatformIO terinstall
- [ ] ESP32 terhubung via USB
- [ ] COM port terdeteksi di Device Manager
- [ ] WiFi credentials sudah diedit di kode
- [ ] IP Backend & AI Service sudah benar
- [ ] Build sukses tanpa error
