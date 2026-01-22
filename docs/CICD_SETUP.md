# GitHub Actions CI/CD Setup Guide
# Cloudflare Pages + AWS Deployment

Panduan lengkap untuk setup CI/CD dengan GitHub Actions untuk arsitektur hybrid (Cloudflare Pages + AWS).

---

## 📋 Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           GitHub Actions CI/CD                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Push to main                                                                │
│       │                                                                      │
│       ├── frontend/** changed ──► Cloudflare Pages (Auto Deploy)            │
│       │                                                                      │
│       └── backend/** changed  ──► SSH to EC2 → Docker Build → Restart       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
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

#### Cloudflare (untuk Frontend)

| Secret Name | Value | Cara Dapat |
|-------------|-------|------------|
| `CLOUDFLARE_API_TOKEN` | `xxx...` | Cloudflare Dashboard → API Tokens → Create Token |
| `CLOUDFLARE_ACCOUNT_ID` | `xxx...` | Cloudflare Dashboard → Overview → Account ID |

#### EC2 Access (untuk Backend)

| Secret Name | Value | Cara Dapat |
|-------------|-------|------------|
| `EC2_BACKEND_HOST` | `54.xxx.xxx.xxx` | EC2 Console → Backend Instance → Public IP |
| `EC2_USERNAME` | `ubuntu` | Default untuk Ubuntu AMI |
| `EC2_SSH_KEY` | `-----BEGIN...-----END...` | Isi file .pem |

**Cara copy EC2_SSH_KEY:**
```bash
cat smart-parking-key.pem
```
Copy semua termasuk `-----BEGIN...` dan `-----END...`

#### Frontend Build Variables

| Secret Name | Value |
|-------------|-------|
| `API_URL` | `https://api.yourdomain.com` |
| `WS_URL` | `wss://api.yourdomain.com/ws` |

---

### Step 3: Connect Cloudflare Pages ke GitHub

1. **Cloudflare Dashboard → Pages → Create a project**
2. **Connect to Git → GitHub**
3. Pilih repository `smart-parking`
4. Configure:
   - **Project name**: `smart-parking`
   - **Production branch**: `main`
   - **Build command**: `npm run build`
   - **Build output directory**: `dist`
   - **Root directory**: `frontend`
5. **Environment variables**:
   - `VITE_API_URL` = `https://api.yourdomain.com`
   - `VITE_WS_URL` = `wss://api.yourdomain.com/ws`
6. **Save and Deploy**

> **Note**: Setelah ini, setiap push ke `main` yang mengubah `frontend/` akan auto-deploy ke Cloudflare Pages!

---

### Step 4: Test Workflow

1. Buat perubahan kecil di code
2. Push ke main:
```bash
git add .
git commit -m "Test CI/CD"
git push
```
3. Cek:
   - **GitHub → Actions** → Lihat workflow running
   - **Cloudflare Dashboard → Pages** → Lihat deployment progress

---

## 📁 Workflow Files

### Deploy Frontend (`.github/workflows/deploy-frontend.yml`)

**Trigger**: Push ke `main` yang mengubah `frontend/**`

```yaml
- Checkout code
- Setup Node.js
- Install dependencies
- Build with environment variables
- Deploy to Cloudflare Pages (cloudflare/pages-action)
```

### Deploy Backend (`.github/workflows/deploy-backend.yml`)

**Trigger**: Push ke `main` yang mengubah `backend/**`

```yaml
- SSH ke EC2 Backend
- Git pull
- Update .env
- Docker build & restart
- Health check
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
gh workflow run deploy-frontend.yml
gh workflow run deploy-backend.yml
```

---

## 🔄 Cloudflare Pages Auto-Purge

Cloudflare Pages otomatis:
- Cache invalidation saat deploy
- Global CDN distribution
- SSL certificate renewal

**Tidak perlu manual purge!**

---

## ❌ Troubleshooting

### Frontend Deploy Failed (Cloudflare)
- Cek Cloudflare Pages → Deployments → View logs
- Pastikan `VITE_API_URL` sudah di-set di Environment variables
- Cek build command: `npm run build`

### Backend Deploy Failed (EC2)
- Cek `EC2_SSH_KEY` sudah benar (termasuk headers)
- Cek Security Group EC2 izinkan SSH dari 0.0.0.0/0
- SSH manual dan cek:
  ```bash
  docker-compose logs backend
  ```

### API Calls Failed from Frontend
- Cek CORS di backend allow domain Cloudflare Pages
- Cek Cloudflare DNS untuk `api.yourdomain.com`

---

## ✅ Checklist

- [ ] Code pushed ke GitHub
- [ ] Repository settings → Actions enabled
- [ ] Cloudflare Pages connected to GitHub
- [ ] Environment variables set di Cloudflare Pages
- [ ] GitHub Secrets untuk EC2 sudah ditambah
- [ ] First push berhasil trigger workflow
- [ ] Frontend live di Cloudflare Pages
- [ ] Backend live di EC2
