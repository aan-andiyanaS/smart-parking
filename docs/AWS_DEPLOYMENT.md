# AWS Deployment Architecture - Smart Parking System

## 📊 Architecture Overview

```
                                    ┌─────────────────┐
                                    │   CloudFront    │
                                    │   (CDN)         │
                                    └────────┬────────┘
                                             │
                    ┌────────────────────────┼────────────────────────┐
                    │                        │                        │
                    ▼                        ▼                        ▼
           ┌────────────────┐      ┌────────────────┐      ┌────────────────┐
           │  EC2 Frontend  │      │  EC2 Backend   │      │      S3        │
           │  (React+Nginx) │      │  (Golang+Docker)│     │  (Images)      │
           │                │──────│                │──────│                │
           │  t2.micro      │      │  t2.small      │      │  parking-img   │
           └────────────────┘      └───────┬────────┘      └────────────────┘
                                           │
                                           ▼
                                  ┌────────────────┐
                                  │  RDS PostgreSQL│
                                  │  (db.t3.micro) │
                                  └────────────────┘
```

---

## 🏗️ AWS Services

| Service | Purpose | Spec |
|---------|---------|------|
| **EC2 Frontend** | React Dashboard + Nginx | t2.micro |
| **EC2 Backend** | Golang API + Docker | t2.small |
| **RDS PostgreSQL** | Database | db.t3.micro |
| **S3** | Image Storage | Standard |
| **CloudFront** | CDN for Frontend | - |

---

## 📁 Repository Structure

```
smart-parking/
├── frontend/           → Deploy ke EC2 Frontend
├── backend/            → Deploy ke EC2 Backend  
├── ai-service/         → Deploy ke EC2 Backend (optional)
├── database/           → Migrate ke RDS
├── nginx/              → Untuk EC2 Frontend
├── .github/workflows/  → CI/CD
└── aws/                → AWS configs
    ├── frontend.docker-compose.yml
    └── backend.docker-compose.yml
```

---

## 🔧 Configuration Changes

### 1. Backend → Use AWS S3 (instead of MinIO)
### 2. Backend → Connect to RDS (instead of local PostgreSQL)
### 3. Frontend → Build static files, serve via Nginx
### 4. CloudFront → CDN for static assets

---

## 📋 Deployment Steps

### Step 1: Create AWS Resources
1. Create VPC + Subnets
2. Create Security Groups
3. Create RDS PostgreSQL
4. Create S3 Bucket
5. Create 2 EC2 instances
6. Create CloudFront distribution

### Step 2: Deploy Backend
1. SSH to EC2 Backend
2. Install Docker
3. Clone repo
4. Configure `.env` with RDS + S3 credentials
5. Run `docker-compose up -d`

### Step 3: Deploy Frontend
1. SSH to EC2 Frontend
2. Install Docker + Nginx
3. Clone repo
4. Build frontend: `npm run build`
5. Serve via Nginx

### Step 4: Configure CloudFront
1. Point to EC2 Frontend
2. Setup SSL certificate
3. Configure caching

---

## 🔐 Security Groups

### Frontend EC2
- Inbound: 80, 443 (HTTP/HTTPS)
- Outbound: All

### Backend EC2
- Inbound: 8080 (from Frontend EC2 only)
- Outbound: All

### RDS PostgreSQL
- Inbound: 5432 (from Backend EC2 only)
