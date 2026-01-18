# 🅿️ Smart Parking System

> Real-time parking slot monitoring system supporting SDG 11: Sustainable Cities and Communities

![Smart Parking](https://img.shields.io/badge/IoT-Smart%20Parking-blue)
![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?logo=go)
![React](https://img.shields.io/badge/React-18-61DAFB?logo=react)
![Docker](https://img.shields.io/badge/Docker-Compose-2496ED?logo=docker)
![Python](https://img.shields.io/badge/Python-3.11+-3776AB?logo=python)

---

## 📋 Overview

Smart Parking System adalah aplikasi IoT untuk monitoring ketersediaan slot parkir secara real-time menggunakan AI computer vision. Sistem ini dirancang untuk mendukung SDG 11 (Sustainable Cities) dengan mengoptimalkan penggunaan area parkir.

### ✨ Features
- ✅ Real-time slot detection dengan AI (YOLOv8)
- ✅ Live camera stream dari ESP32-CAM
- ✅ WebSocket real-time updates
- ✅ Responsive web dashboard
- ✅ Docker deployment support
- ✅ S3-compatible storage (MinIO)

---

## 📚 Dokumentasi

| Dokumen | Deskripsi |
|---------|-----------|
| [📐 Arsitektur Sistem](docs/ARCHITECTURE.md) | Diagram & penjelasan komponen |
| [📖 Manual Guide](docs/MANUAL_GUIDE.md) | Panduan lengkap penggunaan |
| [🔧 Setup Firmware (PlatformIO)](docs/PLATFORMIO_SETUP.md) | Upload firmware via PlatformIO |
| [🔌 Wiring Guide](firmware/WIRING_GUIDE.md) | Skema pengkabelan ESP32 |
| [🔧 Setup Firmware (Arduino)](firmware/arduino/README.md) | Upload firmware via Arduino IDE |
| [☁️ AWS Deployment](docs/AWS_DEPLOYMENT.md) | Deploy ke AWS EC2/RDS/S3 |
| [☁️ AWS Console Setup](docs/AWS_CONSOLE_SETUP.md) | Setup AWS via Console |
| [🔄 CI/CD Setup](docs/CICD_SETUP.md) | GitHub Actions deployment |
| [🌐 Cloudflare Setup](docs/CLOUDFLARE_SETUP.md) | CDN & DNS configuration |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    NGINX (CDN) - Port 80                    │
│   • Static files (React) • API Proxy • WebSocket Proxy     │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────┼────────────────────────────────┐
│                   BACKEND (Golang) - Port 8080              │
│         • REST API • WebSocket • Image Upload               │
└────────────────────────────┬────────────────────────────────┘
                             │
       ┌─────────────────────┼─────────────────────┐
       ▼                     ▼                     ▼
┌─────────────┐     ┌───────────────┐     ┌─────────────┐
│ PostgreSQL  │     │    MinIO      │     │ AI Service  │
│   :5432     │     │    :9000      │     │   :5000     │
└─────────────┘     └───────────────┘     └─────────────┘
                                                 │
                                          ┌──────┴──────┐
                                          │  ESP32-CAM  │
                                          │   Stream    │
                                          └─────────────┘
```

---

## 🔄 Cara Kerja Aplikasi

### System Architecture

![System Architecture](docs/images/system_architecture.png)

### Workflow Diagram

![Smart Parking Workflow](docs/images/smart_parking_flow.png)

**Penjelasan Flow:**

1. **ESP32-CAM** → Streaming video ke AI Service
2. **AI Service (Python/YOLOv8)** → Mendeteksi kendaraan dan mengirim hasil ke Backend
3. **Backend (Golang)** → Update database PostgreSQL dan broadcast via WebSocket
4. **Web Dashboard (React)** → Menerima update real-time dan menampilkan ke user

---

## �🖥️ Prerequisites

| Software | Versi | Kebutuhan |
|----------|-------|-----------|
| Docker Desktop | 4.0+ | Mode Docker |
| Node.js | 18+ | Semua mode |
| Go | 1.21+ | Mode Manual |
| Python | 3.11+ | AI Service |
| VS Code + PlatformIO | Latest | Firmware |

---

## 🚀 Quick Start

### Mode 1: Docker Development (Paling Mudah)

Cara paling cepat untuk menjalankan seluruh sistem dengan Docker Compose.

#### Step 1: Setup Environment

```powershell
# Copy environment file
copy .env.example .env
```

#### Step 2: Build Frontend

```powershell
cd frontend
npm install
npm run build
cd ..
```

#### Step 3: Start All Services

```powershell
# Jalankan semua container
docker-compose up -d

# Verifikasi status
docker-compose ps
```

#### Step 4: Access Application

| Service | URL |
|---------|-----|
| **Dashboard** | http://localhost |
| **API** | http://localhost/api/slots |
| **MinIO Console** | http://localhost:9001 |

> **MinIO Credentials**: `minioadmin` / `minioadmin123`

---

### Mode 2: Manual Development (Untuk Development)

Mode ini untuk development aktif dimana Anda bisa mengedit dan hot-reload setiap service.

#### Step 1: Start Database (PostgreSQL + MinIO)

```powershell
# Jalankan hanya database services
docker-compose up -d postgres minio createbuckets
```

#### Step 2: Setup Environment Variables

```powershell
# PowerShell - Set environment variables
$env:DATABASE_URL="postgres://postgres:postgres123@localhost:5432/smartparking?sslmode=disable"
$env:MINIO_ENDPOINT="localhost:9000"
$env:MINIO_ACCESS_KEY="minioadmin"
$env:MINIO_SECRET_KEY="minioadmin123"
$env:MINIO_BUCKET="parking-images"
$env:ESP32_STREAM_URL="http://192.168.1.50"  # Ganti dengan IP ESP32
$env:AI_SERVICE_URL="http://localhost:5000"
```

#### Step 3: Start Backend (Go)

```powershell
cd backend
go mod tidy
go run cmd/server/main.go
```

> Backend akan berjalan di `http://localhost:8080`

#### Step 4: Start Frontend (React)

```powershell
# Terminal baru
cd frontend
npm install
npm run dev
```

> Frontend akan berjalan di `http://localhost:3000` dengan proxy ke backend

#### Step 5: Start AI Service (Python)

```powershell
# Terminal baru
cd ai-service
pip install -r requirements.txt
python main.py
```

> AI Service akan berjalan di `http://localhost:5000`

#### Step 6: Access Dashboard

Buka browser ke `http://localhost:3000`

---

### Mode 3: Firmware Setup (ESP32)

Setup firmware untuk ESP32-CAM yang akan mengirim video stream ke sistem.

#### Opsi A: Arduino IDE (Lebih Mudah)
📖 Lihat: [firmware/arduino/README.md](firmware/arduino/README.md)

#### Opsi B: PlatformIO (Lebih Advanced)
📖 Lihat: [docs/PLATFORMIO_SETUP.md](docs/PLATFORMIO_SETUP.md)

#### Wiring
📖 Lihat: [firmware/WIRING_GUIDE.md](firmware/WIRING_GUIDE.md)

---

## 🔧 Environment Variables

Edit file `.env` sesuai kebutuhan:

```env
# Database
DATABASE_URL=postgres://postgres:postgres123@localhost:5432/smartparking?sslmode=disable

# MinIO (S3 Storage)
MINIO_ENDPOINT=localhost:9000
MINIO_ACCESS_KEY=minioadmin
MINIO_SECRET_KEY=minioadmin123
MINIO_BUCKET=parking-images

# ESP32 Camera Stream URL (ganti dengan IP ESP32 Anda)
ESP32_STREAM_URL=http://192.168.1.50

# AI Service
AI_SERVICE_URL=http://localhost:5000
```

---

## ❌ Troubleshooting

### "Failed to fetch data. Is the backend running?"

**Penyebab**: Frontend tidak bisa terhubung ke backend API.

**Solusi**:
```powershell
# Cek apakah backend berjalan
netstat -ano | findstr :8080

# Jika menggunakan Mode Manual, pastikan environment variables ter-set
# dan jalankan backend:
cd backend
go run cmd/server/main.go

# Jika menggunakan Docker, cek logs:
docker-compose logs backend
```

### "No parking slots configured"

**Penyebab**: Database belum memiliki data slot parkir.

**Solusi**: Slot parkir perlu dikonfigurasi melalui AI Service annotation tool atau API.

### "Camera not connected" / Stream tidak muncul

**Penyebab**: ESP32_STREAM_URL tidak benar atau ESP32 tidak terhubung.

**Solusi**:
1. Cek IP ESP32 melalui Serial Monitor
2. Update `ESP32_STREAM_URL` di `.env` dengan IP yang benar
3. Pastikan ESP32 dan laptop terhubung ke WiFi yang sama

### "AI detection not working"

**Penyebab**: AI Service tidak berjalan atau belum dikonfigurasi.

**Solusi**:
```powershell
# Cek apakah AI Service berjalan
netstat -ano | findstr :5000

# Jalankan AI Service:
cd ai-service
python main.py
```

### Container tidak berjalan (Docker)

```powershell
# Lihat logs container
docker-compose logs backend
docker-compose logs postgres

# Restart container
docker-compose restart backend

# Reset semua (hapus data)
docker-compose down -v
docker-compose up -d
```

---

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/slots` | Get all parking slots |
| GET | `/api/slots/:id` | Get slot by ID |
| PUT | `/api/slots/:id` | Update slot status |
| POST | `/api/slots/:id/toggle` | Toggle slot occupied |
| GET | `/api/slots/stats` | Get parking statistics |
| POST | `/api/capture` | Upload camera image |
| GET | `/api/capture/latest` | Get latest capture |
| GET | `/api/stream` | Proxy ESP32 stream |
| WS | `/ws` | WebSocket connection |

---

## 📁 Project Structure

```
smart-parking/
├── backend/                 # Golang API server
│   ├── cmd/server/main.go
│   └── internal/
├── frontend/                # React dashboard
│   ├── src/
│   └── vite.config.js
├── ai-service/              # Python AI detection
│   ├── main.py
│   └── annotate_slots.py
├── firmware/                # ESP32 firmware
│   ├── arduino/             # Arduino IDE version
│   ├── src/                 # PlatformIO version
│   └── WIRING_GUIDE.md
├── docs/                    # Documentation
├── nginx/                   # Nginx configuration
├── database/                # Database schema
├── docker-compose.yml       # Docker orchestration
└── .env.example             # Environment template
```

---

## 👥 Team

| Role | Responsibility |
|------|----------------|
| Lead Developer | Backend, Docker, Infrastructure |
| Frontend Dev | React Dashboard, UI/UX |
| AI/ML Engineer | Computer Vision, YOLOv8 |
| IoT Engineer | ESP32 Firmware, Hardware |
| Documentation | Architecture docs, Manual Guide |

---

## 📄 License

MIT License - feel free to use for educational purposes.

---

**Built for SDG 11: Sustainable Cities and Communities** 🌆
