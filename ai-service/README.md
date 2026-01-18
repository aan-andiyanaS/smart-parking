# Smart Parking AI Detection Service

Service untuk mendeteksi slot parkir occupied/empty menggunakan YOLOv11.

## 📋 Fitur

- Deteksi kendaraan menggunakan YOLOv11 nano
- Analisis occupancy berdasarkan region parking yang didefinisikan
- REST API untuk integrasi dengan backend
- Auto-update status slot ke database

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

### 2. Define Parking Regions

Pertama, jalankan annotation tool untuk menentukan area parking slot:

```bash
python annotate_slots.py <gambar_parkiran.jpg>
```

**Controls:**
- **Left click**: Tambah titik polygon
- **n**: Simpan slot saat ini, lanjut ke berikutnya
- **r**: Reset slot saat ini
- **s**: Simpan semua ke file
- **u**: Undo slot terakhir
- **q**: Keluar

Output: `parking_regions.json`

#### 🔬 Alternatif: Gunakan Google Colab

Jika Anda tidak memiliki akses ke environment dengan GUI (atau ingin eksperimen terlebih dahulu), bisa menggunakan notebook Colab:

1. Upload `annotate_slots_colab.ipynb` ke Google Colab
2. Upload gambar parkir Anda
3. Klik untuk mendefinisikan region slot
4. Download `parking_regions.json` yang dihasilkan
5. Copy file tersebut ke folder `ai-service/`

[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com)

### 3. Run AI Service

```bash
cp .env.example .env
# Edit .env dengan konfigurasi yang sesuai

python main.py
```

Service akan berjalan di `http://localhost:5000`

## 📡 API Endpoints

### Health Check
```
GET /health

Response:
{
  "status": "ok",
  "model_loaded": true,
  "regions_loaded": true
}
```

### Analyze Image
```
POST /analyze
Content-Type: multipart/form-data

Body:
- image: <file>

Response:
{
  "success": true,
  "vehicles_detected": 3,
  "slot_status": {
    "P1": true,
    "P2": false,
    "P3": true,
    "P4": false
  },
  "timestamp": "2026-01-17T14:00:00"
}
```

### Get Parking Regions
```
GET /regions

Response:
{
  "success": true,
  "regions": [
    {"code": "P1", "points": [[100,100], [200,100], [200,200], [100,200]]},
    ...
  ]
}
```

### Set Parking Regions
```
POST /regions
Content-Type: application/json

Body:
{
  "regions": [
    {"code": "P1", "points": [[100,100], [200,100], [200,200], [100,200]]},
    ...
  ]
}
```

## 🔄 Integration Flow

```
ESP32-CAM → Upload image → Backend → AI Service → Detect vehicles → Update slots
                                        ↓
                              POST /api/slots/:id (update status)
```

## 🐳 Docker

Build dan run dengan Docker:

```bash
docker build -t parking-ai .
docker run -p 5000:5000 -e BACKEND_URL=http://host.docker.internal parking-ai
```

Atau gunakan docker-compose (lihat docker-compose.yml di root project).

## 📊 Model Info

- **Model**: YOLOv11 nano (yolo11n.pt)
- **Classes detected**: car (2), motorcycle (3), bus (5), truck (7)
- **Speed**: ~20ms per image (GPU), ~200ms (CPU)

## ⚙️ Configuration

Edit `.env`:

```ini
BACKEND_URL=http://localhost      # URL backend API
YOLO_MODEL=yolo11n.pt            # YOLO model file
PORT=5000                         # Service port
```
