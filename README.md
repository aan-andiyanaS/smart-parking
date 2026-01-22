# 🅿️ Smart Parking System

> Real-time parking slot monitoring system with AI detection, supporting SDG 11: Sustainable Cities and Communities

![Smart Parking](https://img.shields.io/badge/IoT-Smart%20Parking-blue)
![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?logo=go)
![React](https://img.shields.io/badge/React-18-61DAFB?logo=react)
![Python](https://img.shields.io/badge/Python-3.11+-3776AB?logo=python)
![Cloudflare](https://img.shields.io/badge/Cloudflare-Pages-F38020?logo=cloudflare)
![AWS](https://img.shields.io/badge/AWS-EC2/RDS-FF9900?logo=amazonaws)

---

## 📋 Overview

Smart Parking System adalah aplikasi IoT untuk monitoring ketersediaan slot parkir secara real-time menggunakan AI computer vision (YOLO). Sistem ini dirancang untuk mendukung SDG 11 (Sustainable Cities) dengan mengoptimalkan penggunaan area parkir.

### ✨ Features
- ✅ Real-time slot detection dengan AI (YOLO11)
- ✅ Live camera stream dari ESP32-CAM
- ✅ WebSocket real-time updates
- ✅ Responsive web dashboard
- ✅ Cloudflare CDN deployment
- ✅ AWS cloud infrastructure

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                                 INTERNET                                      │
│                                                                               │
│  ┌──────────────┐                                    ┌────────────────────┐  │
│  │  ESP32-CAM   │                                    │   USER BROWSER     │  │
│  └──────┬───────┘                                    └─────────┬──────────┘  │
│         │                                                      │             │
└─────────┼──────────────────────────────────────────────────────┼─────────────┘
          │                                                      │
          │                        ┌─────────────────────────────▼────────────┐
          │                        │         CLOUDFLARE (FREE)                │
          │                        │  ┌────────────────────────────────────┐  │
          │                        │  │  Cloudflare Pages (Frontend)       │  │
          │                        │  │  • React Dashboard                 │  │
          │                        │  │  • Global CDN                      │  │
          │                        │  └────────────────────────────────────┘  │
          │                        │  ┌────────────────────────────────────┐  │
          │                        │  │  Cloudflare Proxy (API)            │  │
          │                        │  │  • DDoS Protection                 │  │
          │                        │  └──────────────────┬─────────────────┘  │
          │                        └─────────────────────┼────────────────────┘
          │                                              │
          └────────────────────┬─────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                              AWS VPC                                          │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │  PUBLIC SUBNET                                                          │  │
│  │  ┌──────────────────────────────────────────────────────────────────┐  │  │
│  │  │  Backend (Go/Gin) - EC2 t3.micro                                 │  │  │
│  │  │  • API Gateway • Stream Proxy • WebSocket                        │  │  │
│  │  └──────────────────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │  PRIVATE SUBNET                                                         │  │
│  │  ┌────────────────┐  ┌────────────────┐  ┌─────────────────────────┐   │  │
│  │  │  AI Service    │  │  RDS Postgres  │  │  S3 (Images)            │   │  │
│  │  │  (YOLO)        │  │                │  │                         │   │  │
│  │  └────────────────┘  └────────────────┘  └─────────────────────────┘   │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

📖 **Detail arsitektur**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

---

## 📚 Dokumentasi

| Dokumen | Deskripsi |
|---------|-----------|
| [📐 Arsitektur Sistem](docs/ARCHITECTURE.md) | Diagram & penjelasan komponen |
| [☁️ AWS Deployment](docs/AWS_DEPLOYMENT.md) | Deploy ke AWS EC2/RDS/S3 |
| [🌐 Cloudflare Setup](docs/CLOUDFLARE_SETUP.md) | CDN & DNS configuration |
| [🔄 CI/CD Setup](docs/CICD_SETUP.md) | GitHub Actions deployment |
| [📖 Manual Guide](docs/MANUAL_GUIDE.md) | Panduan lengkap penggunaan |
| [🔧 Setup Firmware](firmware/arduino/README.md) | Upload firmware ESP32 |

---

## 🚀 Quick Start (Local Development)

### Prerequisites

| Software | Versi |
|----------|-------|
| Node.js | 18+ |
| Go | 1.21+ |
| Python | 3.11+ |
| Docker Desktop | 4.0+ |

### Step 1: Start Infrastructure (Docker)

```bash
# Start PostgreSQL + MinIO
docker-compose up -d postgres minio createbuckets
```

### Step 2: Start Backend

```bash
cd backend
cp ../.env.example ../.env
go mod tidy
go run cmd/server/main.go
```

### Step 3: Start AI Service

```bash
cd ai-service
pip install -r requirements.txt
python main.py
```

### Step 4: Start Frontend

```bash
cd frontend
npm install
npm run dev
```

### Access

| Service | URL |
|---------|-----|
| Frontend | http://localhost:3000 |
| Backend API | http://localhost:8080 |
| AI Service | http://localhost:5000 |

---

## 🌐 Production Deployment

| Component | Platform | URL |
|-----------|----------|-----|
| Frontend | Cloudflare Pages | parking.yourdomain.com |
| Backend | AWS EC2 | api.yourdomain.com |
| AI Service | AWS EC2 (Private) | Internal |
| Database | AWS RDS | Private |
| Storage | AWS S3 | Private |

📖 **Setup guide**: [docs/AWS_DEPLOYMENT.md](docs/AWS_DEPLOYMENT.md)

---

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/slots` | Get all parking slots |
| GET | `/api/slots/stats` | Get parking statistics |
| GET | `/api/stream` | Proxy ESP32 camera stream |
| POST | `/api/sessions/entry` | Record vehicle entry |
| POST | `/api/sessions/exit` | Record vehicle exit |
| WS | `/ws` | WebSocket real-time updates |

---

## 📁 Project Structure

```
smart-parking/
├── frontend/          # React dashboard (→ Cloudflare Pages)
├── backend/           # Go API server (→ AWS EC2 Public)
├── ai-service/        # Python YOLO detection (→ AWS EC2 Private)
├── firmware/          # ESP32 camera firmware
├── docs/              # Documentation
├── .github/workflows/ # CI/CD pipelines
└── docker-compose.yml # Local development
```

---

## 💰 Cost ($0 Free Tier)

| Resource | Platform | Cost |
|----------|----------|------|
| Frontend CDN | Cloudflare Pages | **$0** |
| Backend & AI | AWS EC2 t3.micro | Free Tier |
| Database | AWS RDS | Free Tier |
| Storage | AWS S3 | Free Tier |

---

## 📄 License

MIT License - feel free to use for educational purposes.

---

**Built for SDG 11: Sustainable Cities and Communities** 🌆
