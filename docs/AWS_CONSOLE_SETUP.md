# AWS Console Setup Guide - Smart Parking System

Panduan langkah demi langkah untuk deploy Smart Parking System ke AWS.

---

## 📋 Daftar Isi

1. [Login AWS Console](#1-login-aws-console)
2. [Create VPC & Security Groups](#2-create-vpc--security-groups)
3. [Create RDS PostgreSQL](#3-create-rds-postgresql)
4. [Create S3 Bucket](#4-create-s3-bucket)
5. [Create EC2 Backend](#5-create-ec2-backend)
6. [Create EC2 Frontend](#6-create-ec2-frontend)
7. [Create CloudFront](#7-create-cloudfront)
8. [Deploy Application](#8-deploy-application)

---

## 1. Login AWS Console

1. Buka https://aws.amazon.com/console/
2. Klik **Sign In to the Console**
3. Masukkan email dan password
4. Pilih region: **Asia Pacific (Singapore) - ap-southeast-1**

---

## 2. Create VPC & Security Groups

### 2.1 Buat Security Group untuk Backend

1. **Services → EC2 → Security Groups → Create security group**

2. Isi form:
   - **Name**: `smart-parking-backend-sg`
   - **Description**: `Security group for backend EC2`
   - **VPC**: Default VPC

3. **Inbound rules** → Add rule:
   | Type | Port | Source | Description |
   |------|------|--------|-------------|
   | SSH | 22 | My IP | SSH access |
   | Custom TCP | 8080 | 0.0.0.0/0 | Backend API |
   | Custom TCP | 5000 | 10.0.0.0/16 | AI Service (internal) |

4. Klik **Create security group**

### 2.2 Buat Security Group untuk Frontend

1. **Create security group**

2. Isi form:
   - **Name**: `smart-parking-frontend-sg`
   - **Description**: `Security group for frontend EC2`

3. **Inbound rules**:
   | Type | Port | Source | Description |
   |------|------|--------|-------------|
   | SSH | 22 | My IP | SSH access |
   | HTTP | 80 | 0.0.0.0/0 | Web traffic |
   | HTTPS | 443 | 0.0.0.0/0 | SSL traffic |

4. Klik **Create security group**

### 2.3 Buat Security Group untuk RDS

1. **Create security group**

2. Isi form:
   - **Name**: `smart-parking-rds-sg`
   - **Description**: `Security group for RDS PostgreSQL`

3. **Inbound rules**:
   | Type | Port | Source | Description |
   |------|------|--------|-------------|
   | PostgreSQL | 5432 | smart-parking-backend-sg | From backend only |

4. Klik **Create security group**

---

## 3. Create RDS PostgreSQL

1. **Services → RDS → Create database**

2. **Choose a database creation method**: Standard create

3. **Engine options**:
   - Engine type: **PostgreSQL**
   - Version: **16.x** (latest)

4. **Templates**: **Free tier** ✓

5. **Settings**:
   - DB instance identifier: `smart-parking-db`
   - Master username: `postgres`
   - Master password: `YourSecurePassword123!` (catat ini!)

6. **Instance configuration**:
   - DB instance class: **db.t3.micro** (Free tier eligible)

7. **Storage**:
   - Storage type: **gp2**
   - Allocated storage: **20 GB**
   - ❌ Uncheck "Enable storage autoscaling"

8. **Connectivity**:
   - VPC: Default VPC
   - Subnet group: Default
   - Public access: **No**
   - VPC security group: **smart-parking-rds-sg**

9. **Database authentication**: Password authentication

10. **Additional configuration**:
    - Initial database name: `smartparking`
    - ❌ Uncheck "Enable automated backups" (untuk hemat biaya)
    - ❌ Uncheck "Enable encryption"

11. Klik **Create database**

12. **Tunggu 5-10 menit** sampai status "Available"

13. **Catat Endpoint** (contoh: `smart-parking-db.xxxxx.ap-southeast-1.rds.amazonaws.com`)

---

## 4. Create S3 Bucket

1. **Services → S3 → Create bucket**

2. **General configuration**:
   - Bucket name: `smart-parking-images-{unique-id}` (harus unique globally!)
   - Region: **Asia Pacific (Singapore) ap-southeast-1**

3. **Object Ownership**: ACLs enabled

4. **Block Public Access settings**:
   - ❌ **Uncheck** "Block all public access"
   - ✓ Check acknowledgment

5. Klik **Create bucket**

6. **Klik bucket yang baru dibuat → Permissions → Bucket policy**

7. Paste policy ini:
```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "PublicReadGetObject",
            "Effect": "Allow",
            "Principal": "*",
            "Action": "s3:GetObject",
            "Resource": "arn:aws:s3:::smart-parking-images-{unique-id}/*"
        }
    ]
}
```
Ganti `{unique-id}` dengan nama bucket kamu!

8. Klik **Save changes**

---

## 5. Create EC2 Backend

1. **Services → EC2 → Launch instance**

2. **Name**: `smart-parking-backend`

3. **Application and OS Images**:
   - Quick Start: **Ubuntu**
   - AMI: **Ubuntu Server 22.04 LTS (HVM), SSD**
   - Architecture: **64-bit (x86)**

4. **Instance type**: **t2.small** (untuk YOLO, butuh 2GB RAM)

5. **Key pair**:
   - Create new key pair
   - Name: `smart-parking-key`
   - Type: RSA
   - Format: .pem
   - **Download dan simpan dengan aman!**

6. **Network settings**:
   - VPC: Default
   - Subnet: Default
   - Auto-assign public IP: **Enable**
   - Security group: **smart-parking-backend-sg**

7. **Configure storage**: 20 GB gp2

8. Klik **Launch instance**

9. **Catat Public IP** setelah instance running

---

## 6. Create EC2 Frontend

1. **Services → EC2 → Launch instance**

2. **Name**: `smart-parking-frontend`

3. **Application and OS Images**: Ubuntu Server 22.04 LTS

4. **Instance type**: **t2.micro** (cukup untuk static files)

5. **Key pair**: Pilih `smart-parking-key` yang sudah dibuat

6. **Network settings**:
   - Security group: **smart-parking-frontend-sg**
   - Auto-assign public IP: **Enable**

7. **Configure storage**: 10 GB gp2

8. Klik **Launch instance**

9. **Catat Public IP**

---

## 7. Create CloudFront

1. **Services → CloudFront → Create distribution**

2. **Origin**:
   - Origin domain: **Pilih EC2 Frontend Public DNS**
   - Protocol: **HTTP only**
   - Origin path: (kosongkan)

3. **Default cache behavior**:
   - Viewer protocol policy: **Redirect HTTP to HTTPS**
   - Allowed HTTP methods: **GET, HEAD**
   - Cache policy: **CachingOptimized**

4. **Settings**:
   - Price class: **Use only North America and Europe** (termurah)
   - Alternate domain name (CNAME): (kosongkan untuk sekarang)
   - Default root object: `index.html`

5. Klik **Create distribution**

6. **Tunggu 5-15 menit** sampai status "Deployed"

7. **Catat Distribution domain name** (contoh: `d1234abcd.cloudfront.net`)

---

## 8. Deploy Application

### 8.1 Connect ke EC2 Backend

```bash
# Windows (PowerShell dengan OpenSSH)
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_EC2_PUBLIC_IP

# Atau gunakan PuTTY (convert .pem ke .ppk dulu)
```

### 8.2 Setup Backend

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER

# Logout dan login lagi
exit

# Login lagi
ssh -i "smart-parking-key.pem" ubuntu@BACKEND_EC2_PUBLIC_IP

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

# Clone repository
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking

# Create .env file
cat > .env << EOF
DATABASE_URL=postgres://postgres:YourSecurePassword123!@smart-parking-db.xxxxx.ap-southeast-1.rds.amazonaws.com:5432/smartparking?sslmode=require
USE_AWS_S3=true
AWS_REGION=ap-southeast-1
AWS_ACCESS_KEY_ID=YOUR_ACCESS_KEY
AWS_SECRET_ACCESS_KEY=YOUR_SECRET_KEY
S3_BUCKET=smart-parking-images-xxx
PORT=8080
GIN_MODE=release
EOF

# Start backend
docker-compose -f aws/backend.docker-compose.yml up -d --build

# Verify
curl http://localhost:8080/api/slots
```

### 8.3 Connect ke EC2 Frontend

```bash
ssh -i "smart-parking-key.pem" ubuntu@FRONTEND_EC2_PUBLIC_IP
```

### 8.4 Setup Frontend

```bash
# Update & install
sudo apt update && sudo apt upgrade -y
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER

# Install Node.js
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs

# Logout & login lagi
exit
ssh -i "smart-parking-key.pem" ubuntu@FRONTEND_EC2_PUBLIC_IP

# Clone & build
git clone https://github.com/YOUR_USERNAME/smart-parking.git
cd smart-parking/frontend
npm install
npm run build
cd ..

# Update nginx config dengan IP backend
BACKEND_IP="10.0.x.x"  # Private IP dari EC2 Backend
sed -i "s/BACKEND_EC2_PRIVATE_IP/$BACKEND_IP/g" nginx/frontend.nginx.conf

# Start frontend
docker-compose -f aws/frontend.docker-compose.yml up -d

# Verify
curl http://localhost
```

---

## 9. Test Application

1. Buka browser → `http://FRONTEND_EC2_PUBLIC_IP`
2. Atau gunakan CloudFront URL → `https://d1234abcd.cloudfront.net`

---

## 💰 Estimasi Biaya (Per Bulan)

| Service | Spec | Biaya |
|---------|------|-------|
| EC2 Backend | t2.small | ~$17 |
| EC2 Frontend | t2.micro | ~$9 (atau free tier) |
| RDS PostgreSQL | db.t3.micro | ~$13 (atau free tier) |
| S3 | 5GB storage | ~$0.12 |
| CloudFront | 50GB transfer | ~$4 |
| **Total** | | **~$30-45/bulan** |

> **Tip**: Untuk demo/tugas, bisa gunakan Free Tier eligible resources dan matikan saat tidak dipakai!

---

## ✅ Checklist

- [ ] Security Groups created
- [ ] RDS PostgreSQL running
- [ ] S3 Bucket created with public read policy
- [ ] EC2 Backend launched (t2.small)
- [ ] EC2 Frontend launched (t2.micro)
- [ ] CloudFront distribution created
- [ ] Backend deployed and running
- [ ] Frontend deployed and running
- [ ] Application accessible via CloudFront URL
