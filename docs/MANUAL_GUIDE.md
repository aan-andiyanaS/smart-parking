# 📖 Smart Parking System - Manual Guide

## Panduan Lengkap Setup dan Penggunaan

---

## 📋 Daftar Isi

1. [Persyaratan Sistem](#1-persyaratan-sistem)
2. [Instalasi](#2-instalasi)
3. [Menjalankan Aplikasi](#3-menjalankan-aplikasi)
4. [Penggunaan Dashboard](#4-penggunaan-dashboard)
5. [API Documentation](#5-api-documentation)
6. [Troubleshooting](#6-troubleshooting)
7. [Deployment ke AWS](#7-deployment-ke-aws)

---

## 1. Persyaratan Sistem

### Software yang Dibutuhkan

| Software | Versi Minimum | Download |
|----------|---------------|----------|
| Docker Desktop | 4.0+ | [Download](https://www.docker.com/products/docker-desktop) |
| Node.js | 18+ | [Download](https://nodejs.org/) |
| Git | 2.0+ | [Download](https://git-scm.com/) |

### Spesifikasi Hardware

- **RAM**: Minimum 4GB (8GB recommended)
- **Storage**: Minimum 2GB free space
- **OS**: Windows 10/11, macOS, Linux

---

## 2. Instalasi

### Step 1: Clone/Download Project

```bash
# Masuk ke folder project
cd smart-parking
```

### Step 2: Setup Environment

```bash
# Copy file konfigurasi
copy .env.example .env
```

### Step 3: Install Frontend Dependencies

```bash
cd frontend
npm install
```

### Step 4: Build Frontend

```bash
npm run build
cd ..
```

---

## 3. Menjalankan Aplikasi

### Start dengan Docker Compose

```bash
# Jalankan semua services
docker-compose up -d
```

### Verifikasi Services

```bash
# Cek status container
docker-compose ps
```

Output yang diharapkan:
```
NAME                COMMAND                  SERVICE    STATUS
parking-backend     "./main"                 backend    running
parking-nginx       "nginx -g ..."           nginx      running
parking-postgres    "docker-entrypoint..."   postgres   running (healthy)
parking-minio       "minio server ..."       minio      running
```

### Akses Aplikasi

| Layanan | URL | Keterangan |
|---------|-----|------------|
| Dashboard | http://localhost | Halaman utama |
| API | http://localhost/api/slots | REST API |
| MinIO Console | http://localhost:9001 | Storage management |

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
│  │  │   🚗    │   ✅    │   │   │   [Latest Image]  │    │   │
│  │  ├─────────┼─────────┤   │   │                   │    │   │
│  │  │   P3    │   P4    │   │   └───────────────────┘    │   │
│  │  │   ✅    │   🚗    │   │   [Upload Image]           │   │
│  │  └─────────┴─────────┘   │                             │   │
│  └──────────────────────────┴─────────────────────────────┘   │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

### 4.2 Fitur Utama

#### Toggle Slot Status
1. Klik pada slot yang ingin diubah
2. Status akan berubah (Occupied ↔ Available)
3. Perubahan akan disinkronkan ke semua client via WebSocket

#### Upload Gambar
1. Klik tombol "Upload Image"
2. Pilih file gambar (JPG/PNG)
3. Atau drag & drop gambar langsung ke area kamera

#### Real-time Updates
- Dashboard akan menerima update otomatis via WebSocket
- Indikator koneksi: 🟢 Connected / 🔴 Disconnected

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
      "is_occupied": false,
      "position_x": 0,
      "position_y": 0,
      "updated_at": "2026-01-17T12:00:00Z"
    }
  ]
}
```

### 5.2 Toggle Slot

```http
POST /api/slots/{id}/toggle
```

**Response:**
```json
{
  "success": true,
  "data": {
    "id": "uuid",
    "code": "P1",
    "is_occupied": true
  }
}
```

### 5.3 Get Statistics

```http
GET /api/slots/stats
```

**Response:**
```json
{
  "success": true,
  "data": {
    "total": 4,
    "occupied": 2,
    "available": 2,
    "occupancy_rate": 50.0
  }
}
```

### 5.4 Upload Image

```http
POST /api/capture
Content-Type: multipart/form-data

image: [file]
camera_id: cam1
```

**Response:**
```json
{
  "success": true,
  "data": {
    "id": "uuid",
    "image_url": "http://localhost:9000/parking-images/...",
    "camera_id": "cam1",
    "captured_at": "2026-01-17T12:00:00Z"
  }
}
```

### 5.5 WebSocket

```javascript
// Connect
const ws = new WebSocket('ws://localhost/ws')

// Receive messages
ws.onmessage = (event) => {
  const message = JSON.parse(event.data)
  console.log(message)
  // { type: "slot_update", data: {...} }
  // { type: "new_capture", data: {...} }
}
```

---

## 6. Troubleshooting

### 6.1 Container tidak berjalan

```bash
# Cek logs
docker-compose logs backend

# Restart container
docker-compose restart backend
```

### 6.2 Database connection error

```bash
# Cek PostgreSQL status
docker-compose logs postgres

# Restart database
docker-compose restart postgres
```

### 6.3 Frontend tidak loading

```bash
# Rebuild frontend
cd frontend
npm run build
cd ..

# Restart nginx
docker-compose restart nginx
```

### 6.4 MinIO storage error

```bash
# Akses MinIO console
# http://localhost:9001
# Login: minioadmin / minioadmin123

# Pastikan bucket 'parking-images' ada
```

### 6.5 Reset semua data

```bash
# Stop dan hapus semua data
docker-compose down -v

# Start fresh
docker-compose up -d
```

---

## 7. Deployment ke AWS

### 7.1 Menggunakan EC2

#### Step 1: Launch EC2 Instance
- AMI: Amazon Linux 2023
- Type: t2.medium (minimum)
- Security Group: Allow port 80, 8080, 9000, 9001

#### Step 2: Install Docker

```bash
# SSH ke EC2
ssh -i your-key.pem ec2-user@your-ip

# Install Docker
sudo yum update -y
sudo yum install docker -y
sudo systemctl start docker
sudo usermod -aG docker ec2-user

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```

#### Step 3: Deploy Application

```bash
# Clone project
git clone <your-repo> smart-parking
cd smart-parking

# Setup environment
cp .env.example .env
nano .env  # Edit sesuai kebutuhan

# Build frontend
cd frontend && npm install && npm run build && cd ..

# Start services
docker-compose up -d
```

### 7.2 Environment Variables untuk Production

```env
# .env (production)
DATABASE_URL=postgres://user:password@rds-endpoint:5432/smartparking
MINIO_ENDPOINT=s3.amazonaws.com
MINIO_ACCESS_KEY=your-aws-access-key
MINIO_SECRET_KEY=your-aws-secret-key
MINIO_BUCKET=your-s3-bucket
MINIO_USE_SSL=true
GIN_MODE=release
```

---

## 📞 Support

Jika mengalami masalah, silakan:
1. Cek bagian [Troubleshooting](#6-troubleshooting)
2. Buka issue di repository
3. Hubungi tim development

---

**Smart Parking System** - Mendukung SDG 11: Sustainable Cities 🌆
