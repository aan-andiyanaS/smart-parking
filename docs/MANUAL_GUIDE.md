# 📖 Smart Parking System - Manual Guide

## Panduan Lengkap Setup dan Penggunaan

---

## 📋 Daftar Isi

1. [Persyaratan Sistem](#1-persyaratan-sistem)
2. [Instalasi Local](#2-instalasi-local)
3. [Menjalankan Aplikasi](#3-menjalankan-aplikasi)
4. [Penggunaan Dashboard](#4-penggunaan-dashboard)
5. [API Documentation](#5-api-documentation)
6. [Troubleshooting](#6-troubleshooting)
7. [Deployment ke Cloud](#7-deployment-ke-cloud)

---

## 1. Persyaratan Sistem

### Software yang Dibutuhkan

| Software | Versi Minimum | Download |
|----------|---------------|----------|
| Docker Desktop | 4.0+ | [Download](https://www.docker.com/products/docker-desktop) |
| Node.js | 18+ | [Download](https://nodejs.org/) |
| Go | 1.21+ | [Download](https://golang.org/) |
| Python | 3.11+ | [Download](https://python.org/) |
| Git | 2.0+ | [Download](https://git-scm.com/) |

### Spesifikasi Hardware

- **RAM**: Minimum 4GB (8GB recommended)
- **Storage**: Minimum 2GB free space
- **OS**: Windows 10/11, macOS, Linux

---

## 2. Instalasi Local

### Step 1: Clone Project

```bash
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking
```

### Step 2: Setup Environment

```bash
# Copy file konfigurasi
copy .env.example .env
```

### Step 3: Install Dependencies

```bash
# Frontend
cd frontend && npm install && cd ..

# AI Service
cd ai-service && pip install -r requirements.txt && cd ..

# Backend
cd backend && go mod tidy && cd ..
```

---

## 3. Menjalankan Aplikasi

### Mode 1: Docker Compose (Paling Mudah)

```bash
# Start database services
docker-compose up -d postgres minio createbuckets

# Build frontend
cd frontend && npm run build && cd ..

# Start backend
docker-compose up -d backend nginx
```

### Mode 2: Manual (Untuk Development)

**Terminal 1 - Backend:**
```bash
cd backend
go run cmd/server/main.go
```

**Terminal 2 - AI Service:**
```bash
cd ai-service
python main.py
```

**Terminal 3 - Frontend:**
```bash
cd frontend
npm run dev
```

### Akses Aplikasi

| Layanan | URL |
|---------|-----|
| Dashboard | http://localhost:3000 (dev) / http://localhost (Docker) |
| API | http://localhost:8080/api/slots |
| AI Service | http://localhost:5000 |
| MinIO Console | http://localhost:9001 (minioadmin/minioadmin123) |

---

## 4. Penggunaan Dashboard

### 4.1 Overview Dashboard

```
┌────────────────────────────────────────────────────────────────┐
│  🅿️ Smart Parking System                      🟢 Connected    │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌────────────────┐ ┌────────────────┐ ┌────────────────┐     │
│  │   🅿️ 4 Total   │ │  🚗 2 Occupied │ │  ✅ 2 Available │     │
│  └────────────────┘ └────────────────┘ └────────────────┘     │
│                                                                │
│  ┌──────────────────────────┬─────────────────────────────┐   │
│  │      PARKING SLOTS       │       LIVE CAMERA           │   │
│  │  ┌─────────┬─────────┐   │   ┌───────────────────┐    │   │
│  │  │   P1    │   P2    │   │   │                   │    │   │
│  │  │   🚗    │   ✅    │   │   │   [Live Stream]   │    │   │
│  │  ├─────────┼─────────┤   │   │                   │    │   │
│  │  │   P3    │   P4    │   │   └───────────────────┘    │   │
│  │  │   ✅    │   🚗    │   │   [Auto] [Detect] [Upload] │   │
│  │  └─────────┴─────────┘   │                             │   │
│  └──────────────────────────┴─────────────────────────────┘   │
└────────────────────────────────────────────────────────────────┘
```

### 4.2 Fitur Utama

| Fitur | Deskripsi |
|-------|-----------|
| **Toggle Slot** | Klik slot untuk ubah status manual |
| **Live Stream** | Video real-time dari ESP32-CAM |
| **Auto Detection** | AI mendeteksi kendaraan setiap 5 detik |
| **Manual Detection** | Trigger deteksi manual saat auto OFF |
| **Upload Image** | Upload gambar untuk dianalisis |
| **Real-time Updates** | WebSocket update ke semua client |

---

## 5. API Documentation

### 5.1 Get All Slots

```http
GET /api/slots
```

**Response:**
```json
{
  "success": true,
  "data": [
    {
      "id": "uuid",
      "code": "P1",
      "zone": "A",
      "is_occupied": false
    }
  ]
}
```

### 5.2 Get Statistics

```http
GET /api/slots/stats
```

### 5.3 Toggle Slot

```http
POST /api/slots/{id}/toggle
```

### 5.4 Stream Proxy

```http
GET /api/stream          # MJPEG stream
GET /api/stream/capture  # Single frame
```

### 5.5 Sessions (Entry/Exit)

```http
POST /api/sessions/entry  # Vehicle entry
POST /api/sessions/exit   # Vehicle exit
```

### 5.6 WebSocket

```javascript
const ws = new WebSocket('ws://localhost:8080/ws')

ws.onmessage = (event) => {
  const message = JSON.parse(event.data)
  // { type: "slot_update", data: {...} }
  // { type: "detection_overlay", data: {...} }
}
```

---

## 6. Troubleshooting

### 6.1 Backend tidak berjalan

```bash
# Cek logs
docker-compose logs backend

# Atau manual
cd backend && go run cmd/server/main.go
```

### 6.2 AI Service error

```bash
# Cek Python dependencies
cd ai-service
pip install -r requirements.txt
python main.py
```

### 6.3 Frontend not loading

```bash
cd frontend
npm install
npm run dev
```

### 6.4 Database connection error

```bash
# Cek PostgreSQL
docker-compose logs postgres

# Restart
docker-compose restart postgres
```

### 6.5 Reset semua data

```bash
docker-compose down -v
docker-compose up -d
```

---

## 7. Deployment ke Cloud

### Arsitektur Production

```
┌────────────────────────────────────────────────────────────────────┐
│                          PRODUCTION                                 │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    CLOUDFLARE                                │   │
│  │  ┌─────────────────────┐  ┌─────────────────────────────┐   │   │
│  │  │  Cloudflare Pages   │  │  Cloudflare Proxy           │   │   │
│  │  │  (Frontend React)   │  │  (API Protection)           │   │   │
│  │  │  parking.domain.com │  │  api.domain.com             │   │   │
│  │  └─────────────────────┘  └──────────────┬──────────────┘   │   │
│  └──────────────────────────────────────────┼───────────────────┘   │
│                                              │                       │
│  ┌──────────────────────────────────────────▼───────────────────┐   │
│  │                        AWS VPC                                │   │
│  │  ┌───────────────────────────────────────────────────────┐   │   │
│  │  │  PUBLIC SUBNET                                         │   │   │
│  │  │  EC2 Backend (Go/Gin)                                  │   │   │
│  │  └─────────────────────────┬─────────────────────────────┘   │   │
│  │                             │                                 │   │
│  │  ┌─────────────────────────▼─────────────────────────────┐   │   │
│  │  │  PRIVATE SUBNET                                        │   │   │
│  │  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │   │   │
│  │  │  │ AI Service   │ │     RDS      │ │      S3      │   │   │   │
│  │  │  │ (YOLO)       │ │  PostgreSQL  │ │   Images     │   │   │   │
│  │  │  └──────────────┘ └──────────────┘ └──────────────┘   │   │   │
│  │  └────────────────────────────────────────────────────────┘   │   │
│  └───────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────┘
```

### Deployment Guides

| Guide | Deskripsi |
|-------|-----------|
| [AWS_DEPLOYMENT.md](AWS_DEPLOYMENT.md) | Arsitektur & environment |
| [AWS_CONSOLE_SETUP.md](AWS_CONSOLE_SETUP.md) | Step-by-step AWS setup |
| [CLOUDFLARE_SETUP.md](CLOUDFLARE_SETUP.md) | Cloudflare Pages setup |
| [CICD_SETUP.md](CICD_SETUP.md) | GitHub Actions CI/CD |

### Quick Deploy Steps

1. **Setup Cloudflare Pages** (Frontend)
   - Connect GitHub repo
   - Auto-deploy on push

2. **Setup AWS** (Backend)
   - Create VPC with public/private subnets
   - Deploy EC2 instances
   - Configure RDS & S3

3. **Configure DNS**
   - `parking.domain.com` → Cloudflare Pages
   - `api.domain.com` → EC2 Backend (via Cloudflare)

---

## 📞 Support

Jika mengalami masalah:
1. Cek [Troubleshooting](#6-troubleshooting)
2. Buka issue di repository
3. Cek dokumentasi di folder `docs/`

---

**Smart Parking System** - Mendukung SDG 11: Sustainable Cities 🌆
