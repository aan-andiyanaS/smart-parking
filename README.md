# 🅿️ Smart Parking System

> Real-time parking slot monitoring system supporting SDG 11: Sustainable Cities and Communities

![Smart Parking](https://img.shields.io/badge/IoT-Smart%20Parking-blue)
![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?logo=go)
![React](https://img.shields.io/badge/React-18-61DAFB?logo=react)
![Docker](https://img.shields.io/badge/Docker-Compose-2496ED?logo=docker)

## 📋 Overview

Smart Parking System adalah aplikasi IoT untuk monitoring ketersediaan slot parkir secara real-time. Sistem ini dirancang untuk mendukung SDG 11 (Sustainable Cities) dengan mengoptimalkan penggunaan area parkir.

### Features
- ✅ Real-time slot monitoring
- ✅ Live camera capture
- ✅ WebSocket updates
- ✅ Responsive dashboard
- ✅ Docker deployment
- ✅ S3-compatible storage (MinIO)

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
│ PostgreSQL  │     │    MinIO      │     │   Redis     │
│   :5432     │     │    :9000      │     │  (optional) │
└─────────────┘     └───────────────┘     └─────────────┘
```

## 🚀 Quick Start

### Prerequisites
- Docker Desktop
- Git

### 1. Clone & Setup

```bash
# Navigate to project
cd smart-parking

# Copy environment file
cp .env.example .env
```

### 2. Build Frontend

```bash
cd frontend
npm install
npm run build
cd ..
```

### 3. Start Services

```bash
# Start all containers
docker-compose up -d

# Check status
docker-compose ps

# View logs
docker-compose logs -f backend
```

### 4. Access Application

| Service | URL |
|---------|-----|
| **Dashboard** | http://localhost |
| **API** | http://localhost/api/slots |
| **MinIO Console** | http://localhost:9001 |

MinIO credentials: `minioadmin` / `minioadmin123`

## 📁 Project Structure

```
smart-parking/
├── docker-compose.yml       # Docker orchestration
├── .env.example             # Environment template
│
├── backend/                 # Golang API server
│   ├── Dockerfile
│   ├── go.mod
│   ├── cmd/server/main.go
│   └── internal/
│       ├── config/
│       ├── database/
│       ├── handlers/
│       ├── models/
│       └── services/
│
├── frontend/                # React dashboard
│   ├── Dockerfile
│   ├── package.json
│   └── src/
│       ├── App.jsx
│       ├── components/
│       └── services/
│
├── nginx/                   # Nginx configuration
│   └── nginx.conf
│
├── database/                # Database schema
│   └── init.sql
│
└── docs/                    # Documentation
    └── MANUAL_GUIDE.md
```

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
| WS | `/ws` | WebSocket connection |

## 🔧 Development

### Backend (Golang)

```bash
cd backend

# Download dependencies
go mod tidy

# Run locally
go run cmd/server/main.go
```

### Frontend (React)

```bash
cd frontend

# Install dependencies
npm install

# Development server
npm run dev

# Build for production
npm run build
```

## 🐳 Docker Commands

```bash
# Start all services
docker-compose up -d

# Rebuild specific service
docker-compose up -d --build backend

# View logs
docker-compose logs -f

# Stop all services
docker-compose down

# Remove volumes (clean start)
docker-compose down -v
```

## ☁️ AWS Deployment

### Using EC2 + Docker

```bash
# On EC2 instance
sudo yum install docker docker-compose-plugin -y
sudo systemctl start docker

# Clone project
git clone <your-repo>
cd smart-parking

# Start services
docker compose up -d
```

### Using ECS

1. Push images to ECR
2. Create ECS Task Definition
3. Create ECS Service
4. Configure ALB for load balancing

## 👥 Team

| Role | Responsibility |
|------|----------------|
| Lead Developer | Backend, Docker, Infrastructure |
| Frontend Dev | React Dashboard, UI/UX |
| Documentation | Architecture docs, Manual Guide |
| QA/Testing | Testing, Demo preparation |

## 📄 License

MIT License - feel free to use for educational purposes.

---

**Built for SDG 11: Sustainable Cities and Communities** 🌆
