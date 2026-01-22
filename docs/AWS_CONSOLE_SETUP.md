# AWS Console Setup Guide - Smart Parking System
# 💰 OPTIMIZED FOR FREE TIER
# 📡 Using Cloudflare Pages for Frontend (FREE)

Panduan langkah demi langkah untuk deploy Smart Parking System ke AWS dengan **biaya minimal**.

---

## 💰 Estimasi Biaya dengan Free Tier

| Service | Spec | Free Tier | Biaya/Bulan |
|---------|------|-----------|-------------|
| Cloudflare Pages | Frontend | Unlimited | **$0** |
| EC2 Backend | t3.micro | 750 jam/bulan | **$0** |
| EC2 AI Service | t3.micro | 750 jam/bulan | **$0** |
| RDS PostgreSQL | db.t3.micro | 750 jam/bulan | **$0** |
| S3 | 5GB | 5GB free | **$0** |
| **Total** | | | **$0/bulan** |

> ⚠️ **Note**: 2 EC2 instances running 24/7 = 1440 jam/bulan (melebihi 750 jam free tier). Solusi: Matikan saat tidak dipakai, atau gabung Backend+AI di 1 EC2.

---

## 📋 Daftar Isi

1. [Login AWS Console](#1-login-aws-console)
2. [Create VPC & Subnets](#2-create-vpc--subnets)
3. [Create Security Groups](#3-create-security-groups)
4. [Create RDS PostgreSQL](#4-create-rds-postgresql)
5. [Create S3 Bucket](#5-create-s3-bucket)
6. [Create EC2 Backend (Public)](#6-create-ec2-backend-public)
7. [Create EC2 AI Service (Private)](#7-create-ec2-ai-service-private)
8. [Setup Cloudflare Pages (Frontend)](#8-setup-cloudflare-pages-frontend)
9. [Deploy Application](#9-deploy-application)

---

## 1. Login AWS Console

1. Buka https://aws.amazon.com/console/
2. Klik **Sign In to the Console**
3. Pilih region: **Asia Pacific (Singapore) - ap-southeast-1**

---

## 2. Create VPC & Subnets

### 2.1 Create VPC

1. **Services → VPC → Create VPC**
2. Settings:
   - Name: `smart-parking-vpc`
   - IPv4 CIDR: `10.0.0.0/16`
3. Create VPC

### 2.2 Create Public Subnet

1. **Subnets → Create subnet**
   - VPC: `smart-parking-vpc`
   - Name: `smart-parking-public`
   - CIDR: `10.0.1.0/24`
   - AZ: ap-southeast-1a

### 2.3 Create Private Subnet

1. **Create subnet**
   - Name: `smart-parking-private`
   - CIDR: `10.0.2.0/24`
   - AZ: ap-southeast-1a

### 2.4 Create Internet Gateway

1. **Internet Gateways → Create**
   - Name: `smart-parking-igw`
2. **Attach to VPC**: `smart-parking-vpc`

### 2.5 Create Route Table (Public)

1. **Route Tables → Create**
   - Name: `smart-parking-public-rt`
   - VPC: `smart-parking-vpc`
2. **Edit routes**: Add `0.0.0.0/0` → `smart-parking-igw`
3. **Subnet associations**: Associate `smart-parking-public`

### 2.6 Create S3 VPC Endpoint (FREE)

1. **Endpoints → Create endpoint**
   - Service: `com.amazonaws.ap-southeast-1.s3` (Gateway)
   - VPC: `smart-parking-vpc`
   - Route tables: Both public and private
2. **Cost: $0** (Gateway endpoints are free!)

---

## 3. Create Security Groups

### 3.1 Backend Security Group (Public)

1. **EC2 → Security Groups → Create**
   - Name: `sg-backend`
   - VPC: `smart-parking-vpc`

2. **Inbound rules**:
   | Type | Port | Source | Description |
   |------|------|--------|-------------|
   | SSH | 22 | My IP | Admin access |
   | Custom TCP | 8080 | Cloudflare IPs* | API from Cloudflare |

   *Cloudflare IPs: https://www.cloudflare.com/ips/

### 3.2 AI Service Security Group (Private)

1. **Create security group**
   - Name: `sg-ai-service`

2. **Inbound rules**:
   | Type | Port | Source |
   |------|------|--------|
   | Custom TCP | 5000 | sg-backend |

### 3.3 RDS Security Group

1. **Create security group**
   - Name: `sg-rds`

2. **Inbound rules**:
   | Type | Port | Source |
   |------|------|--------|
   | PostgreSQL | 5432 | sg-backend |

---

## 4. Create RDS PostgreSQL

1. **Services → RDS → Create database**
2. **Engine**: PostgreSQL 16
3. **Templates**: ⭐ **Free tier**
4. **Settings**:
   - DB identifier: `smart-parking-db`
   - Username: `postgres`
   - Password: (catat!)
5. **Instance**: **db.t3.micro**
6. **Storage**: 20 GB, disable autoscaling
7. **Connectivity**:
   - VPC: `smart-parking-vpc`
   - Subnet: Create new (private subnets)
   - Public access: **No**
   - Security group: `sg-rds`
8. **Additional**: 
   - Database name: `smartparking`
   - Disable backup, encryption, monitoring
9. **Create database**

---

## 5. Create S3 Bucket

1. **Services → S3 → Create bucket**
2. **Name**: `smart-parking-images-xxx` (unique)
3. **Region**: ap-southeast-1
4. **Block Public Access**: Keep default (private)
5. **Create bucket**

> Note: Backend accesses S3 via VPC Endpoint (private, no internet)

---

## 6. Create EC2 Backend (Public)

1. **EC2 → Launch instance**
2. **Name**: `smart-parking-backend`
3. **AMI**: Ubuntu Server 22.04 LTS
4. **Instance type**: ⭐ **t3.micro**
5. **Key pair**: Create `smart-parking-key` → Download .pem
6. **Network**:
   - VPC: `smart-parking-vpc`
   - Subnet: `smart-parking-public`
   - Auto-assign public IP: **Enable**
   - Security group: `sg-backend`
7. **Storage**: 8 GB gp3
8. **Launch instance**

---

## 7. Create EC2 AI Service (Private)

1. **Launch instance**
2. **Name**: `smart-parking-ai`
3. **AMI**: Ubuntu Server 22.04 LTS
4. **Instance type**: **t3.micro**
5. **Key pair**: `smart-parking-key`
6. **Network**:
   - Subnet: `smart-parking-private`
   - Auto-assign public IP: **Disable**
   - Security group: `sg-ai-service`
7. **Storage**: 8 GB gp3
8. **Launch instance**

> ⚠️ **Access**: No public IP. Use AWS Session Manager or SSH via Backend.

---

## 8. Setup Cloudflare Pages (Frontend)

> 📖 Detail: [CLOUDFLARE_SETUP.md](CLOUDFLARE_SETUP.md)

### Quick Steps:

1. **Cloudflare Dashboard → Pages → Create project**
2. **Connect to Git** → Select `smart-parking` repo
3. **Build settings**:
   - Framework: Vite
   - Build command: `npm run build`
   - Output: `dist`
   - Root: `frontend`
4. **Environment variables**:
   - `VITE_API_URL` = `https://api.yourdomain.com`
   - `VITE_WS_URL` = `wss://api.yourdomain.com/ws`
5. **Save and Deploy**

### DNS Records:

| Type | Name | Content | Proxy |
|------|------|---------|-------|
| CNAME | `parking` | `xxx.pages.dev` | 🟠 Proxied |
| A | `api` | EC2 Backend IP | 🟠 Proxied |

---

## 9. Deploy Application

### 9.1 Setup Backend (EC2 Public)

```bash
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_IP

# Install Docker
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER
logout

# Reconnect & clone
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_IP
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking

# Create .env
cat > .env << EOF
DATABASE_URL=postgres://postgres:PASSWORD@RDS_ENDPOINT:5432/smartparking
AI_SERVICE_URL=http://10.0.2.XX:5000
AWS_REGION=ap-southeast-1
S3_BUCKET=smart-parking-images-xxx
PORT=8080
GIN_MODE=release
EOF

# Start backend
docker-compose up -d backend
```

### 9.2 Setup AI Service (EC2 Private)

```bash
# Via Session Manager (recommended)
aws ssm start-session --target i-xxxxx

# Or SSH via Backend
ssh -i key.pem -J ubuntu@BACKEND_IP ubuntu@10.0.2.XX

# Clone & run
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking/ai-service
docker-compose up -d
```

---

## ✅ Deployment Checklist

- [ ] VPC with public/private subnets created
- [ ] S3 VPC Endpoint configured (free)
- [ ] Security groups configured correctly
- [ ] RDS in private subnet
- [ ] EC2 Backend in public subnet with Elastic IP
- [ ] EC2 AI Service in private subnet
- [ ] Cloudflare Pages connected to GitHub
- [ ] DNS records configured
- [ ] Backend accessible via api.yourdomain.com
- [ ] Frontend accessible via parking.yourdomain.com
