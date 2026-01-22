# AWS Deployment Architecture - Smart Parking System

## 📊 Hybrid Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                   INTERNET                                       │
│                                                                                  │
│  ┌────────────────┐                                       ┌──────────────────┐  │
│  │    ESP32-CAM   │                                       │   USER BROWSER   │  │
│  │   MJPEG Stream │                                       │                  │  │
│  └───────┬────────┘                                       └────────┬─────────┘  │
│          │                                                         │            │
└──────────┼─────────────────────────────────────────────────────────┼────────────┘
           │                                                         │
           │                                                         ▼
           │                         ┌─────────────────────────────────────────────┐
           │                         │           CLOUDFLARE (FREE)                 │
           │                         │                                             │
           │                         │  ┌──────────────────────────────────────┐  │
           │                         │  │  CLOUDFLARE PAGES (Frontend)         │  │
           │                         │  │  • parking.yourdomain.com            │  │
           │                         │  │  • Global CDN, Auto SSL              │  │
           │                         │  └──────────────────────────────────────┘  │
           │                         │                    │ API calls             │
           │                         │  ┌─────────────────▼────────────────────┐  │
           │                         │  │  CLOUDFLARE PROXY                    │  │
           │                         │  │  • api.yourdomain.com                │  │
           │                         │  │  • DDoS + WAF Protection             │  │
           │                         │  └──────────────────┬───────────────────┘  │
           │                         └─────────────────────┼───────────────────────┘
           │                                               │
           └───────────────────────┬───────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                  AWS VPC                                         │
│                                                                                  │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │  PUBLIC SUBNET (10.0.1.0/24)                                               │  │
│  │  ┌─────────────────────────────────────────────────────────────────────┐  │  │
│  │  │  EC2 BACKEND (Go/Gin) - t3.micro                                    │  │  │
│  │  │  • API endpoints + Stream proxy                                     │  │  │
│  │  │  • WebSocket for real-time updates                                  │  │  │
│  │  │  • Periodic AI detection (every 5 sec)                              │  │  │
│  │  └─────────────────────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                      │                                           │
│  ┌───────────────────────────────────▼───────────────────────────────────────┐  │
│  │  PRIVATE SUBNET (10.0.2.0/24) - No Internet Access                        │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐   │  │
│  │  │   AI SERVICE    │  │      RDS        │  │    S3 VPC Endpoint      │   │  │
│  │  │  (Python/YOLO)  │  │   PostgreSQL    │  │    parking-images       │   │  │
│  │  │   t3.micro      │  │  db.t3.micro    │  │                         │   │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘   │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🏗️ AWS Services

| Service | Purpose | Spec | Cost |
|---------|---------|------|------|
| **Cloudflare Pages** | Frontend React | CDN Global | **FREE** |
| **Cloudflare Proxy** | DDoS + WAF | - | **FREE** |
| **EC2 Backend** | Golang API + Stream Proxy | t3.micro | Free Tier |
| **EC2 AI Service** | YOLO Detection | t3.micro | Free Tier |
| **RDS PostgreSQL** | Database | db.t3.micro | Free Tier |
| **S3** | Image Storage | Standard | Free Tier |
| **S3 VPC Endpoint** | Private S3 access | Gateway | **FREE** |

**Total: $0/month** (within Free Tier limits)

---

## 📁 Repository Structure

```
smart-parking/
├── frontend/           → Deploy ke Cloudflare Pages
├── backend/            → Deploy ke EC2 (Public Subnet)
├── ai-service/         → Deploy ke EC2 (Private Subnet)
├── database/           → Migrate ke RDS
├── .github/workflows/
│   ├── deploy-frontend.yml  → Cloudflare Pages
│   └── deploy-backend.yml   → AWS EC2
└── docs/
    └── AWS_DEPLOYMENT.md    → This file
```

---

## 🔧 Environment Configuration

### Frontend (.env.production)
```env
VITE_API_URL=https://api.yourdomain.com
VITE_WS_URL=wss://api.yourdomain.com/ws
```

### Backend (EC2)
```env
DATABASE_URL=postgres://user:pass@parking-db.xxx.rds.amazonaws.com:5432/smartparking
AI_SERVICE_URL=http://10.0.2.20:5000
MINIO_ENDPOINT=s3.ap-southeast-1.amazonaws.com
ESP32_STREAM_URL=http://ESP32_IP:81/stream
```

### AI Service (EC2 Private)
```env
BACKEND_URL=http://10.0.1.10:8080
MODEL_PATH=./yolo11n.pt
PORT=5000
```

---

## 🔐 Security Groups

### sg-backend (Public Subnet)
| Direction | Port | Source | Description |
|-----------|------|--------|-------------|
| Inbound | 8080 | Cloudflare IPs | API from Cloudflare |
| Inbound | 8080 | ESP32 IP | Stream from ESP32 |
| Inbound | 22 | Your IP | SSH |
| Outbound | 5000 | sg-ai-service | To AI Service |
| Outbound | 5432 | sg-rds | To Database |

### sg-ai-service (Private Subnet)
| Direction | Port | Source | Description |
|-----------|------|--------|-------------|
| Inbound | 5000 | sg-backend | Only from Backend |

### sg-rds (Private Subnet)
| Direction | Port | Source | Description |
|-----------|------|--------|-------------|
| Inbound | 5432 | sg-backend | Only from Backend |

---

## 📋 Deployment Steps

### Step 1: Setup Cloudflare

1. Add domain to Cloudflare
2. Create Cloudflare Pages project (connect GitHub)
3. Configure DNS:
   ```
   parking  CNAME  xxx.pages.dev        (Proxied)
   api      A      54.xxx.xxx.xxx       (Proxied)
   ```

### Step 2: Setup AWS VPC

1. Create VPC (10.0.0.0/16)
2. Create Public Subnet (10.0.1.0/24) + Internet Gateway
3. Create Private Subnet (10.0.2.0/24)
4. Create S3 VPC Endpoint (Gateway type - FREE)
5. Create Security Groups

### Step 3: Deploy Backend (Public Subnet)

```bash
# SSH to EC2 Backend
ssh -i key.pem ubuntu@54.xxx.xxx.xxx

# Install Docker
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker ubuntu

# Clone and run
git clone https://github.com/yourusername/smart-parking.git
cd smart-parking
docker-compose up -d
```

### Step 4: Deploy AI Service (Private Subnet)

```bash
# SSH via Session Manager (no public IP needed)
aws ssm start-session --target i-xxx

# Run AI Service
cd /home/ubuntu/smart-parking/ai-service
docker-compose up -d
```

### Step 5: Setup CI/CD

1. Add GitHub Secrets:
   - `CLOUDFLARE_API_TOKEN`
   - `CLOUDFLARE_ACCOUNT_ID`
   - `API_URL`
   - `WS_URL`
   - `EC2_SSH_KEY`

2. Push to main branch → Auto deploy

---

## 🌐 DNS Configuration

| Name | Type | Content | Proxy |
|------|------|---------|-------|
| `parking` | CNAME | xxx.pages.dev | 🟠 Yes |
| `api` | A | 54.xxx.xxx.xxx | 🟠 Yes |

---

## 📊 Data Flow

```
ESP32 ──MJPEG──▶ Backend:8080/api/stream ──proxy──▶ Frontend (via WebSocket)

Backend (every 5s):
  └── Capture frame → AI Service → YOLO detect → WebSocket broadcast
```
