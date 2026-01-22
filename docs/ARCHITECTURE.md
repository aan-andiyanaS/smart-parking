# Smart Parking System - Architecture Document

## System Overview

Smart Parking System adalah aplikasi IoT untuk monitoring slot parkir secara real-time menggunakan AI (YOLO) untuk deteksi kendaraan. Sistem ini mendukung SDG 11 (Sustainable Cities and Communities).

## Hybrid Architecture (Cloudflare + AWS)

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                    INTERNET                                          │
│                                                                                      │
│  ┌────────────────┐                                       ┌────────────────────────┐│
│  │    ESP32-CAM   │                                       │      USER BROWSER      ││
│  │   MJPEG Stream │                                       │                        ││
│  └───────┬────────┘                                       └───────────┬────────────┘│
│          │                                                            │             │
└──────────┼────────────────────────────────────────────────────────────┼─────────────┘
           │                                                            │
           │                                                            ▼
           │                         ┌──────────────────────────────────────────────────┐
           │                         │              CLOUDFLARE (FREE)                   │
           │                         │                                                  │
           │                         │  ┌────────────────────────────────────────────┐ │
           │                         │  │       CLOUDFLARE PAGES (Frontend)          │ │
           │                         │  │       • parking.yourdomain.com             │ │
           │                         │  │       • React Dashboard (Static)           │ │
           │                         │  │       • Global CDN, Auto SSL               │ │
           │                         │  └────────────────────────────────────────────┘ │
           │                         │                      │ API/WS calls             │
           │                         │  ┌───────────────────▼────────────────────────┐ │
           │                         │  │       CLOUDFLARE PROXY                     │ │
           │                         │  │       • api.yourdomain.com                 │ │
           │                         │  │       • DDoS + WAF Protection              │ │
           │                         │  └──────────────────┬─────────────────────────┘ │
           │                         └─────────────────────┼───────────────────────────┘
           │                                               │
           └───────────────────────┬───────────────────────┘
                                   │
                                   ▼
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                                     AWS VPC                                           │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐ │
│  │  PUBLIC SUBNET (10.0.1.0/24)                                                     │ │
│  │  ┌───────────────────────────────────────────────────────────────────────────┐  │ │
│  │  │  EC2 BACKEND (Go/Gin) - t3.micro                                          │  │ │
│  │  │                                                                            │  │ │
│  │  │  Endpoints:                                                                │  │ │
│  │  │  ├── GET  /api/stream       → Proxy MJPEG dari ESP32                      │  │ │
│  │  │  ├── GET  /api/slots        → Get parking slots                          │  │ │
│  │  │  ├── POST /api/sessions/*   → Entry/exit events                          │  │ │
│  │  │  └── WS   /ws               → Real-time updates                          │  │ │
│  │  │                                                                            │  │ │
│  │  │  Background Jobs:                                                          │  │ │
│  │  │  └── Every 5s: Capture frame → AI Service → Broadcast result              │  │ │
│  │  └───────────────────────────────────────────────────────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────────────────────────────────┘ │
│                                        │                                              │
│                            Internal Network (10.0.x.x)                                │
│                                        │                                              │
│  ┌─────────────────────────────────────▼───────────────────────────────────────────┐ │
│  │  PRIVATE SUBNET (10.0.2.0/24) - No Internet Access                              │ │
│  │                                                                                  │ │
│  │  ┌─────────────────────┐  ┌─────────────────────┐  ┌──────────────────────────┐ │ │
│  │  │    AI SERVICE       │  │        RDS          │  │     S3 VPC Endpoint      │ │ │
│  │  │    (Python/YOLO)    │  │    PostgreSQL       │  │                          │ │ │
│  │  │                     │  │                     │  │     parking-images       │ │ │
│  │  │  POST /analyze      │  │  Tables:            │  │     bucket               │ │ │
│  │  │  GET  /overlay      │  │  • slots            │  │                          │ │ │
│  │  │                     │  │  • sessions         │  │                          │ │ │
│  │  │  t3.micro           │  │  • captures         │  │                          │ │ │
│  │  └─────────────────────┘  └─────────────────────┘  └──────────────────────────┘ │ │
│  │                                                                                  │ │
│  │  ❌ No Public IP    ❌ No Internet Gateway    ✅ Only accessible from Backend   │ │
│  └──────────────────────────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────────────────────────┘
```

## Component Description

### 1. Frontend (React + Vite) - Cloudflare Pages
| Aspect | Detail |
|--------|--------|
| **Technology** | React 18, Vite 5 |
| **Hosting** | Cloudflare Pages (CDN) |
| **Deploy** | Auto from GitHub |
| **Features** | Real-time dashboard, Live stream, WebSocket |

### 2. Backend (Golang + Gin) - AWS EC2 Public
| Aspect | Detail |
|--------|--------|
| **Technology** | Go 1.21, Gin, GORM |
| **Instance** | EC2 t3.micro |
| **Location** | Public Subnet |
| **Functions** | API Gateway, Stream Proxy, WebSocket Hub |

### 3. AI Service (Python + YOLO) - AWS EC2 Private
| Aspect | Detail |
|--------|--------|
| **Technology** | Python, Flask, Ultralytics YOLO11 |
| **Instance** | EC2 t3.micro |
| **Location** | Private Subnet (no internet) |
| **Functions** | Vehicle detection, Slot analysis |

### 4. Database (PostgreSQL) - AWS RDS
| Aspect | Detail |
|--------|--------|
| **Technology** | PostgreSQL 16 |
| **Instance** | db.t3.micro |
| **Location** | Private Subnet |
| **Tables** | slots, captures, sessions |

### 5. Storage (S3) - AWS S3
| Aspect | Detail |
|--------|--------|
| **Technology** | AWS S3 |
| **Access** | Via VPC Endpoint (no NAT) |
| **Bucket** | parking-images |

### 6. IoT Device (ESP32-CAM)
| Aspect | Detail |
|--------|--------|
| **Technology** | ESP32 + OV2640 |
| **Protocol** | MJPEG Stream, HTTP POST |
| **Target** | Backend API |

## Data Flow

### Flow 1: Live Video Streaming
```
ESP32-CAM ──MJPEG──▶ Backend:8080/api/stream ──proxy──▶ Frontend (via Cloudflare)
                                                        Latency: ~300-500ms
```

### Flow 2: Periodic AI Detection
```
Backend (every 5 seconds)
    │
    ├── 1. Capture frame from ESP32 stream
    │
    ├── 2. POST to AI Service (10.0.2.20:5000/analyze)
    │       └── YOLO detection
    │       └── Return slot_status
    │
    ├── 3. Save results to RDS
    │
    └── 4. Broadcast via WebSocket ──▶ Frontend updates overlay
```

### Flow 3: User Interaction
```
User Click → Frontend → API Request
                            ↓
                   Cloudflare Proxy
                            ↓
                   Backend (AWS EC2)
                            ↓
                   Query/Update RDS
                            ↓
                   WebSocket Broadcast
                            ↓
                   All Connected Clients
```

## Technology Stack Summary

| Layer | Technology | Location |
|-------|------------|----------|
| Frontend | React + Vite | Cloudflare Pages |
| Backend | Golang + Gin | AWS EC2 (Public) |
| AI Service | Python + YOLO | AWS EC2 (Private) |
| Database | PostgreSQL | AWS RDS (Private) |
| Storage | S3 | AWS S3 (VPC Endpoint) |
| CDN/Proxy | Cloudflare | Cloudflare |
| IoT | ESP32-CAM | Local Network |

## Security Architecture

### Network Security
```
┌───────────────────────────────────────────────────────────────┐
│  SECURITY GROUPS                                               │
│                                                                │
│  sg-backend (Public)                                           │
│  ├── Inbound 8080: Cloudflare IPs + ESP32                     │
│  ├── Inbound 22: YOUR_IP/32 only                              │
│  └── Outbound: AI Service, RDS, S3                            │
│                                                                │
│  sg-ai-service (Private)                                       │
│  └── Inbound 5000: sg-backend only                            │
│                                                                │
│  sg-rds (Private)                                              │
│  └── Inbound 5432: sg-backend only                            │
└───────────────────────────────────────────────────────────────┘
```

### Additional Security
- **CORS** - Configured for allowed origins
- **Cloudflare WAF** - Basic protection (free)
- **DDoS Protection** - Via Cloudflare
- **SSL/TLS** - Auto via Cloudflare
- **Private Subnet** - AI/DB not accessible from internet

## Cost Estimation (Free Tier)

| Resource | Specification | Cost/Month |
|----------|--------------|------------|
| Cloudflare Pages | Unlimited | **$0** |
| Cloudflare Proxy | Free plan | **$0** |
| EC2 x2 | t3.micro | $0 (Free Tier) |
| RDS | db.t3.micro | $0 (Free Tier) |
| S3 | 5GB | $0 (Free Tier) |
| **Total** | | **$0** |

## Deployment

### CI/CD Pipeline
```
GitHub Push (main)
       │
       ├──▶ Frontend → Cloudflare Pages (auto)
       │
       └──▶ Backend/AI → GitHub Actions → EC2 Deploy
```

### Environment Variables

**Frontend** (Cloudflare):
```env
VITE_API_URL=https://api.yourdomain.com
VITE_WS_URL=wss://api.yourdomain.com/ws
```

**Backend** (EC2):
```env
AI_SERVICE_URL=http://10.0.2.20:5000
DATABASE_URL=postgres://...@rds-endpoint:5432/smartparking
```
