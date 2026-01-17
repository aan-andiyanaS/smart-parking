# AWS Console Setup Guide - Smart Parking System
# 💰 OPTIMIZED FOR FREE TIER ($100 Credit)
# 📡 Using Cloudflare as CDN (FREE)

Panduan langkah demi langkah untuk deploy Smart Parking System ke AWS dengan **biaya minimal**.

---

## 💰 Estimasi Biaya dengan Free Tier

| Service | Spec | Free Tier | Biaya/Bulan |
|---------|------|-----------|-------------|
| EC2 Backend | t2.micro | 750 jam/bulan (1 tahun) | **$0** |
| EC2 Frontend | t2.micro | 750 jam/bulan (1 tahun) | **$0** |
| RDS PostgreSQL | db.t3.micro | 750 jam/bulan (1 tahun) | **$0** |
| S3 | 5GB | 5GB free | **$0** |
| Cloudflare CDN | Unlimited | Always free | **$0** |
| **Total** | | | **$0/bulan** |

> ⚠️ **Dengan $100 credit, bisa jalan sangat lama!**

---

## 📋 Daftar Isi

1. [Login AWS Console](#1-login-aws-console)
2. [Create Security Groups](#2-create-security-groups)
3. [Create RDS PostgreSQL (Free Tier)](#3-create-rds-postgresql-free-tier)
4. [Create S3 Bucket (Free)](#4-create-s3-bucket-free)
5. [Create EC2 Backend (Free Tier)](#5-create-ec2-backend-free-tier)
6. [Create EC2 Frontend (Free Tier)](#6-create-ec2-frontend-free-tier)
7. [Setup Cloudflare CDN (Free)](#7-setup-cloudflare-cdn-free)
8. [Deploy Application](#8-deploy-application)
9. [Tips Hemat Biaya](#9-tips-hemat-biaya)

---

## 1. Login AWS Console

1. Buka https://aws.amazon.com/console/
2. Klik **Sign In to the Console**
3. Masukkan email dan password
4. Pilih region: **Asia Pacific (Singapore) - ap-southeast-1**

---

## 2. Create Security Groups

### 2.1 Backend Security Group

1. **Services → EC2 → Security Groups → Create security group**

2. Isi:
   - **Name**: `smart-parking-backend-sg`
   - **Description**: `Backend EC2`
   - **VPC**: Default VPC

3. **Inbound rules**:
   | Type | Port | Source |
   |------|------|--------|
   | SSH | 22 | My IP |
   | Custom TCP | 8080 | 0.0.0.0/0 |

4. Klik **Create**

### 2.2 Frontend Security Group

1. **Create security group**
   - **Name**: `smart-parking-frontend-sg`

2. **Inbound rules**:
   | Type | Port | Source |
   |------|------|--------|
   | SSH | 22 | My IP |
   | HTTP | 80 | 0.0.0.0/0 |
   | HTTPS | 443 | 0.0.0.0/0 |

### 2.3 RDS Security Group

1. **Create security group**
   - **Name**: `smart-parking-rds-sg`

2. **Inbound rules**:
   | Type | Port | Source |
   |------|------|--------|
   | PostgreSQL | 5432 | smart-parking-backend-sg |

---

## 3. Create RDS PostgreSQL (Free Tier)

1. **Services → RDS → Create database**

2. **Method**: Standard create

3. **Engine**: PostgreSQL 16.x

4. **Templates**: ⭐ **Free tier** ⭐

5. **Settings**:
   - DB identifier: `smart-parking-db`
   - Username: `postgres`
   - Password: `YourPassword123!` (catat!)

6. **Instance**: 
   - ⭐ **db.t3.micro** (Free tier eligible)

7. **Storage**:
   - Type: gp2
   - Size: **20 GB**
   - ❌ Disable autoscaling

8. **Connectivity**:
   - Public access: **No**
   - Security group: `smart-parking-rds-sg`

9. **Additional**:
   - Database name: `smartparking`
   - ❌ Disable automated backups
   - ❌ Disable encryption
   - ❌ Disable monitoring

10. **Create database**

11. Catat **Endpoint** setelah available

---

## 4. Create S3 Bucket (Free)

1. **Services → S3 → Create bucket**

2. **Name**: `smart-parking-images-xxx` (unique)

3. **Region**: ap-southeast-1

4. **Object Ownership**: ACLs enabled

5. **Block Public Access**: ❌ Uncheck all

6. **Create bucket**

7. **Bucket → Permissions → Bucket policy**:
```json
{
    "Version": "2012-10-17",
    "Statement": [{
        "Sid": "PublicRead",
        "Effect": "Allow",
        "Principal": "*",
        "Action": "s3:GetObject",
        "Resource": "arn:aws:s3:::smart-parking-images-xxx/*"
    }]
}
```

---

## 5. Create EC2 Backend (Free Tier)

1. **Services → EC2 → Launch instance**

2. **Name**: `smart-parking-backend`

3. **AMI**: Ubuntu Server 22.04 LTS (Free tier eligible)

4. **Instance type**: ⭐ **t2.micro** (Free tier) ⭐
   > ⚠️ Catatan: t2.micro (1GB RAM) tidak cukup untuk AI Service. Untuk demo tanpa AI, gunakan t2.micro. Jika butuh AI, upgrade ke t2.small (+$9/bulan).

5. **Key pair**: Create new → `smart-parking-key` → Download .pem

6. **Network**:
   - Auto-assign public IP: **Enable**
   - Security group: `smart-parking-backend-sg`

7. **Storage**: **8 GB** gp2 (hemat storage)

8. **Launch instance**

---

## 6. Create EC2 Frontend (Free Tier)

1. **Launch instance**

2. **Name**: `smart-parking-frontend`

3. **AMI**: Ubuntu Server 22.04 LTS

4. **Instance type**: ⭐ **t2.micro** (Free tier) ⭐

5. **Key pair**: `smart-parking-key`

6. **Network**:
   - Security group: `smart-parking-frontend-sg`
   - Auto-assign public IP: **Enable**

7. **Storage**: **8 GB** gp2

8. **Launch instance**

---

## 7. Setup Cloudflare CDN (Free)

> 📖 Lihat dokumentasi lengkap: `docs/CLOUDFLARE_SETUP.md`

### Ringkasan:

1. **Daftar** di https://dash.cloudflare.com/sign-up

2. **Add site** → masukkan domain kamu

3. **Update nameservers** di domain registrar

4. **Add DNS records**:
   | Type | Name | Content | Proxy |
   |------|------|---------|-------|
   | A | `@` | EC2 Frontend IP | ☁️ Proxied |
   | A | `www` | EC2 Frontend IP | ☁️ Proxied |
   | A | `api` | EC2 Backend IP | ☁️ Proxied |

5. **SSL/TLS** → Mode: **Full** atau **Flexible**

**Hasil:**
- `yourdomain.com` → Frontend dengan HTTPS gratis!
- `api.yourdomain.com` → Backend

---

## 8. Deploy Application

### 8.1 Setup EC2 Backend

```bash
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_IP

# Install Docker
sudo apt update && sudo apt upgrade -y
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER

# Logout & login lagi
exit
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_IP

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

# Clone repo
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking

# Create .env
cat > .env << EOF
DATABASE_URL=postgres://postgres:YourPassword123!@RDS_ENDPOINT:5432/smartparking?sslmode=require
USE_AWS_S3=true
AWS_REGION=ap-southeast-1
AWS_ACCESS_KEY_ID=xxx
AWS_SECRET_ACCESS_KEY=xxx
S3_BUCKET=smart-parking-images-xxx
PORT=8080
GIN_MODE=release
EOF

# Start (TANPA AI SERVICE untuk t2.micro)
docker-compose -f aws/backend.docker-compose.yml up -d backend

# Verify
curl http://localhost:8080/api/slots
```

### 8.2 Setup EC2 Frontend

```bash
ssh -i "smart-parking-key.pem" ubuntu@FRONTEND_IP

# Install Docker & Node
sudo apt update && sudo apt upgrade -y
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs

# Logout & login
exit
ssh -i "smart-parking-key.pem" ubuntu@FRONTEND_IP

# Clone & build
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking/frontend
npm install
npm run build
cd ..

# Update nginx dengan Backend Private IP
sed -i "s/BACKEND_EC2_PRIVATE_IP/PRIVATE_IP/g" nginx/frontend.nginx.conf

# Start
docker-compose -f aws/frontend.docker-compose.yml up -d
```

---

## 9. Tips Hemat Biaya 💰

### ✅ Yang Harus Dilakukan

1. **Matikan EC2 saat tidak dipakai**
   ```bash
   # AWS Console → EC2 → Instance → Stop
   ```

2. **Gunakan Free Tier resources**
   - t2.micro untuk EC2
   - db.t3.micro untuk RDS
   - 5GB S3
   - Cloudflare (gratis selamanya!)

3. **Disable fitur yang tidak perlu**
   - RDS: backup, encryption, monitoring

4. **Set Billing Alerts**
   - AWS Console → Billing → Budgets
   - Create budget: $10/month alert

### ❌ Yang Harus Dihindari

1. **Jangan biarkan EC2 running 24/7** jika tidak perlu
2. **Jangan pakai instance besar** (t2.small, t2.medium)
3. **Jangan enable semua monitoring**
4. **Jangan lupa terminate** setelah selesai demo

### 📊 Monitor Biaya

1. **AWS Console → Billing → Bills**
2. Cek usage setiap minggu
3. Set budget alert di $20, $50, $80

---

## ✅ Checklist Free Tier

- [ ] EC2 Backend: t2.micro ✓
- [ ] EC2 Frontend: t2.micro ✓
- [ ] RDS: db.t3.micro ✓
- [ ] Storage: 8GB per EC2 ✓
- [ ] S3: Public bucket ✓
- [ ] Cloudflare: DNS + CDN setup ✓
- [ ] Billing alert set ✓
