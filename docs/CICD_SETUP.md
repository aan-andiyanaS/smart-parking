# GitHub Actions CI/CD Setup Guide
# 📡 Using Cloudflare as CDN

Panduan lengkap untuk setup CI/CD dengan GitHub Actions.

---

## 📋 Overview

```
┌─────────────────────────────────────────────────────────┐
│                   GitHub Actions                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Push to main                                           │
│       │                                                 │
│       ├── backend/** changed  ──► Deploy Backend EC2   │
│       │                                                 │
│       └── frontend/** changed ──► Build ──► Deploy EC2 │
│                                                         │
│  Cloudflare automatically caches new content!          │
└─────────────────────────────────────────────────────────┘
```

---

## 🚀 Step-by-Step Setup

### Step 1: Push Code ke GitHub

```bash
cd smart-parking

# Init git (jika belum)
git init

# Add semua files
git add .

# Commit
git commit -m "Initial commit"

# Buat repository di GitHub, lalu:
git remote add origin https://github.com/USERNAME/smart-parking.git
git branch -M main
git push -u origin main
```

---

### Step 2: Setup GitHub Secrets

1. Buka repository di GitHub
2. **Settings → Secrets and variables → Actions**
3. Klik **New repository secret**
4. Tambah secrets berikut:

#### EC2 Access

| Secret Name | Value | Cara Dapat |
|-------------|-------|------------|
| `EC2_BACKEND_HOST` | `13.250.xxx.xxx` | EC2 Console → Backend Instance → Public IP |
| `EC2_FRONTEND_HOST` | `52.77.xxx.xxx` | EC2 Console → Frontend Instance → Public IP |
| `EC2_USERNAME` | `ubuntu` | Default untuk Ubuntu AMI |
| `EC2_SSH_KEY` | `-----BEGIN RSA PRIVATE KEY-----...` | Isi dari file .pem |

**Untuk EC2_SSH_KEY:**
```bash
# Buka file .pem dan copy isinya
cat smart-parking-key.pem
```
Copy semua termasuk `-----BEGIN...` dan `-----END...`

#### Database

| Secret Name | Value |
|-------------|-------|
| `DATABASE_URL` | `postgres://postgres:PASSWORD@rds-endpoint:5432/smartparking?sslmode=require` |

#### AWS Credentials (untuk S3 only)

| Secret Name | Value | Cara Dapat |
|-------------|-------|------------|
| `AWS_REGION` | `ap-southeast-1` | - |
| `AWS_ACCESS_KEY_ID` | `AKIA...` | IAM → Users → Security credentials |
| `AWS_SECRET_ACCESS_KEY` | `xxx...` | IAM → Users → Security credentials |
| `S3_BUCKET` | `smart-parking-images-xxx` | S3 Console |

#### Frontend Build

| Secret Name | Value |
|-------------|-------|
| `API_URL` | `https://api.yourdomain.com` |
| `WS_URL` | `wss://api.yourdomain.com/ws` |

---

### Step 3: Create IAM User untuk CI/CD

1. **Services → IAM → Users → Add user**
2. User name: `github-actions`
3. Access type: **Access key - Programmatic access** ✓
4. Attach policies:
   - `AmazonS3FullAccess`
5. Create user
6. **Copy Access Key ID dan Secret Access Key!**

---

### Step 4: Test Workflow

1. Buat perubahan kecil di code
2. Push ke main:
```bash
git add .
git commit -m "Test CI/CD"
git push
```
3. Buka **GitHub → Actions** → Lihat workflow running

---

## 📁 Workflow Files

### CI Workflow (`.github/workflows/ci.yml`)

Jalan pada: Setiap push dan PR
```yaml
- Build Golang backend
- Lint Go code
- Build React frontend
- Test Docker build
```

### Deploy Backend (`.github/workflows/deploy-backend.yml`)

Jalan pada: Push ke main yang mengubah `backend/**`
```yaml
- SSH ke EC2 Backend
- Git pull
- Update .env
- Docker build & restart
- Health check
```

### Deploy Frontend (`.github/workflows/deploy-frontend.yml`)

Jalan pada: Push ke main yang mengubah `frontend/**`
```yaml
- Build React
- SSH ke EC2 Frontend
- Copy files
- Restart Nginx
- Cloudflare auto-caches new content
```

---

## 🔧 Manual Trigger

Jika mau trigger deployment manual:

**Via GitHub UI:**
1. **Actions** tab
2. Pilih workflow
3. **Run workflow** → Select branch → Run

**Via GitHub CLI:**
```bash
gh workflow run deploy-backend.yml
gh workflow run deploy-frontend.yml
```

---

## 🔄 Cloudflare Cache

Cloudflare otomatis cache content baru. Jika perlu purge manual:

1. **Cloudflare Dashboard → Caching → Purge Everything**

Atau via API (opsional, bisa ditambah ke workflow):
```bash
curl -X POST "https://api.cloudflare.com/client/v4/zones/ZONE_ID/purge_cache" \
     -H "Authorization: Bearer CLOUDFLARE_API_TOKEN" \
     -H "Content-Type: application/json" \
     --data '{"purge_everything":true}'
```

---

## ❌ Troubleshooting

### SSH Connection Failed
- Cek `EC2_SSH_KEY` sudah benar (termasuk headers)
- Cek Security Group EC2 izinkan SSH dari 0.0.0.0/0
- Cek EC2 instance running

### Docker Build Failed
- SSH ke EC2 dan cek log:
  ```bash
  docker-compose -f aws/backend.docker-compose.yml logs
  ```

### Cache Not Updated
- Cloudflare → Caching → Development Mode → On (disable cache sementara)
- Atau Purge Everything

---

## ✅ Checklist

- [ ] Code pushed ke GitHub
- [ ] Repository settings → Actions enabled
- [ ] Semua secrets sudah ditambah
- [ ] IAM user untuk CI/CD dibuat
- [ ] First push berhasil trigger workflow
- [ ] Backend deploy success
- [ ] Frontend deploy success
- [ ] Cloudflare DNS configured
