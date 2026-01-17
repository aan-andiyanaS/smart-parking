# Smart Parking System - Architecture Document

## System Overview

Smart Parking System adalah aplikasi IoT untuk monitoring slot parkir secara real-time yang mendukung SDG 11 (Sustainable Cities and Communities).

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SMART PARKING SYSTEM                            │
│                             System Architecture                              │
└─────────────────────────────────────────────────────────────────────────────┘

                                    ┌──────────┐
                                    │   USER   │
                                    │ Browser  │
                                    └────┬─────┘
                                         │ HTTP/WebSocket
                                         │ Port 80
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│   ┌──────────────────────────────────────────────────────────────────────┐  │
│   │                       NGINX (CDN / Reverse Proxy)                     │  │
│   │                              Port 80                                   │  │
│   │                                                                        │  │
│   │   ┌─────────────────────┐  ┌─────────────────────┐                   │  │
│   │   │   Static Files      │  │    Proxy Rules      │                   │  │
│   │   │   /index.html       │  │    /api/* → :8080   │                   │  │
│   │   │   /assets/*         │  │    /ws   → :8080    │                   │  │
│   │   │   (React Build)     │  │    /images/* → MinIO│                   │  │
│   │   └─────────────────────┘  └──────────┬──────────┘                   │  │
│   │                                       │                               │  │
│   └───────────────────────────────────────┼────────────────────────────────┘  │
│                                           │                                   │
│   ┌───────────────────────────────────────┼────────────────────────────────┐  │
│   │                     BACKEND (Golang + Gin)                              │  │
│   │                          Port 8080                                      │  │
│   │                                                                         │  │
│   │  ┌─────────────────────────────────────────────────────────────────┐   │  │
│   │  │                        HTTP Handlers                             │   │  │
│   │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐   │   │  │
│   │  │  │ SlotHandler  │  │CaptureHandler│  │  WebSocketHandler    │   │   │  │
│   │  │  │ - GetAll     │  │ - Upload     │  │  - Real-time updates │   │   │  │
│   │  │  │ - Toggle     │  │ - GetLatest  │  │  - Broadcast         │   │   │  │
│   │  │  │ - GetStats   │  │              │  │                      │   │   │  │
│   │  │  └──────┬───────┘  └──────┬───────┘  └──────────────────────┘   │   │  │
│   │  │         │                 │                                      │   │  │
│   │  └─────────┼─────────────────┼──────────────────────────────────────┘   │  │
│   │            │                 │                                          │  │
│   │  ┌─────────┼─────────────────┼──────────────────────────────────────┐   │  │
│   │  │         │    Services     │                                      │   │  │
│   │  │  ┌──────▼───────┐  ┌──────▼───────┐  ┌─────────────────────┐    │   │  │
│   │  │  │ GORM (ORM)   │  │ MinIO Client │  │   WebSocket Hub     │    │   │  │
│   │  │  │ - Models     │  │ - Upload     │  │   - Connections     │    │   │  │
│   │  │  │ - Queries    │  │ - Delete     │  │   - Broadcast       │    │   │  │
│   │  │  └──────┬───────┘  └──────┬───────┘  └─────────────────────┘    │   │  │
│   │  │         │                 │                                      │   │  │
│   │  └─────────┼─────────────────┼──────────────────────────────────────┘   │  │
│   │            │                 │                                          │  │
│   └────────────┼─────────────────┼──────────────────────────────────────────┘  │
│                │                 │                                              │
│   ┌────────────┼─────────────────┼──────────────────────────────────────────┐  │
│   │            │  DATA LAYER     │                                          │  │
│   │            │                 │                                          │  │
│   │  ┌─────────▼───────┐  ┌──────▼──────────┐                              │  │
│   │  │   PostgreSQL    │  │     MinIO       │                              │  │
│   │  │    Port 5432    │  │   Port 9000     │                              │  │
│   │  │                 │  │                  │                              │  │
│   │  │  ┌───────────┐  │  │  ┌────────────┐ │                              │  │
│   │  │  │   slots   │  │  │  │  Images    │ │                              │  │
│   │  │  │ captures  │  │  │  │  Bucket    │ │                              │  │
│   │  │  │ sessions  │  │  │  └────────────┘ │                              │  │
│   │  │  └───────────┘  │  │                  │                              │  │
│   │  └─────────────────┘  └──────────────────┘                              │  │
│   │                                                                          │  │
│   └──────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
│                              DOCKER COMPOSE                                      │
│                          (Container Orchestration)                               │
│                                                                                  │
└──────────────────────────────────────────────────────────────────────────────────┘
                                         ▲
                                         │ HTTP POST /api/capture
                                         │
                               ┌─────────┴─────────┐
                               │    ESP32-CAM      │
                               │  (IoT Device)     │
                               │                   │
                               │  • OV2640 Camera  │
                               │  • WiFi Module    │
                               │  • Auto Capture   │
                               └───────────────────┘
```

## Component Description

### 1. Frontend (React + Vite)
- **Technology**: React 18, Vite 5
- **Purpose**: Dashboard untuk monitoring
- **Features**:
  - Real-time slot grid
  - Live camera view
  - Statistics display
  - WebSocket integration

### 2. Backend (Golang + Gin)
- **Technology**: Go 1.21, Gin Framework, GORM
- **Purpose**: REST API dan WebSocket server
- **Endpoints**:
  - `/api/slots` - CRUD parking slots
  - `/api/capture` - Image upload
  - `/ws` - WebSocket connection

### 3. Database (PostgreSQL)
- **Technology**: PostgreSQL 16
- **Tables**:
  - `slots` - Parking slot data
  - `captures` - Camera captures
  - `sessions` - Parking sessions

### 4. Storage (MinIO)
- **Technology**: MinIO (S3-compatible)
- **Purpose**: Image storage
- **Bucket**: `parking-images`

### 5. CDN/Proxy (Nginx)
- **Technology**: Nginx Alpine
- **Purpose**: 
  - Serve static files
  - Reverse proxy to backend
  - WebSocket proxy
  - Image caching

### 6. IoT Device (ESP32-CAM)
- **Technology**: ESP32 + OV2640
- **Purpose**: Capture parking area images
- **Protocol**: HTTP POST

## Data Flow

### Slot Update Flow
```
User Click → Frontend → HTTP PUT /api/slots/:id
                              ↓
                      Backend (Golang)
                              ↓
                      Update PostgreSQL
                              ↓
                      Broadcast WebSocket
                              ↓
                      All Connected Clients
```

### Image Capture Flow
```
ESP32-CAM → HTTP POST /api/capture
                   ↓
            Backend (Golang)
                   ↓
            Upload to MinIO
                   ↓
            Save record to PostgreSQL
                   ↓
            Broadcast WebSocket
                   ↓
            Dashboard updates
```

## Technology Stack Summary

| Layer | Technology | Version |
|-------|------------|---------|
| Frontend | React + Vite | 18 + 5 |
| Backend | Golang + Gin | 1.21 |
| Database | PostgreSQL | 16 |
| Storage | MinIO | Latest |
| Proxy/CDN | Nginx | Alpine |
| Container | Docker Compose | Latest |
| IoT | ESP32-CAM | - |

## Security Considerations

1. **CORS** - Configured for allowed origins
2. **Input Validation** - All inputs validated
3. **File Upload** - Size and type restrictions
4. **Environment Variables** - Secrets in .env file

## Scalability

- Horizontal scaling possible with load balancer
- Database can be replaced with managed RDS
- Storage can be replaced with AWS S3
- Backend can be containerized and replicated
