# Cloudflare CDN Setup Guide

Panduan setup Cloudflare sebagai CDN untuk Smart Parking System.

---

## 💰 Kenapa Cloudflare?

| Fitur | CloudFront | Cloudflare |
|-------|------------|------------|
| Harga | ~$2-5/bulan | **GRATIS** |
| SSL | ✅ | ✅ Gratis |
| CDN | ✅ | ✅ |
| DDoS Protection | ✅ | ✅ |
| Setup | Kompleks | Mudah |

---

## 📋 Langkah Setup

### Step 1: Daftar Cloudflare

1. Buka https://dash.cloudflare.com/sign-up
2. Daftar dengan email
3. Verify email

---

### Step 2: Add Domain

1. Klik **Add site**
2. Masukkan domain kamu: `yourdomain.com`
3. Pilih plan: **Free**
4. Klik **Continue**

---

### Step 3: Update Nameservers

Cloudflare akan memberikan 2 nameservers, contoh:
```
ada.ns.cloudflare.com
bob.ns.cloudflare.com
```

1. Login ke domain registrar (Niagahoster, Namecheap, GoDaddy, dll)
2. Cari **DNS Settings** atau **Nameservers**
3. Ganti nameservers dengan yang dari Cloudflare
4. Tunggu propagasi (bisa sampai 24 jam, biasanya <1 jam)

---

### Step 4: Add DNS Records

Di Cloudflare Dashboard → **DNS** → **Records** → **Add record**

| Type | Name | Content | Proxy |
|------|------|---------|-------|
| A | `@` | EC2 Frontend IP | ☁️ Proxied |
| A | `www` | EC2 Frontend IP | ☁️ Proxied |
| A | `api` | EC2 Backend IP | ☁️ Proxied |
| A | `parking` | EC2 Frontend IP | ☁️ Proxied |

**Hasilnya:**
- `yourdomain.com` → EC2 Frontend
- `www.yourdomain.com` → EC2 Frontend  
- `api.yourdomain.com` → EC2 Backend
- `parking.yourdomain.com` → EC2 Frontend

---

### Step 5: Enable SSL

1. **SSL/TLS** → **Overview**
2. Mode: **Full (strict)**
3. Jika EC2 belum punya SSL, pilih **Flexible**

---

### Step 6: Configure Caching

1. **Caching** → **Configuration**
2. Caching Level: **Standard**
3. Browser Cache TTL: **1 day**

---

### Step 7: Page Rules (Optional)

Untuk SPA React, buat rule agar handle 404:

1. **Rules** → **Page Rules** → **Create Page Rule**
2. URL: `parking.yourdomain.com/*`
3. Settings:
   - Cache Level: **Standard**
   - Always Online: **On**

---

## 🔧 Update Frontend Code

Update `.env` atau `vite.config.js` untuk API URL:

```javascript
// frontend/src/services/api.js
const API_BASE_URL = 'https://api.yourdomain.com';
```

Atau di build:
```bash
VITE_API_URL=https://api.yourdomain.com npm run build
```

---

## 🔧 Update Nginx Config

Edit `nginx/frontend.nginx.conf` untuk Backend:

```nginx
upstream backend_api {
    server api.yourdomain.com:8080;  # Atau EC2 Private IP
}
```

Jika menggunakan Cloudflare proxy untuk backend:
```nginx
upstream backend_api {
    server 127.0.0.1:8080;  # Jika frontend & backend di 1 EC2
}
```

---

## ✅ Test

1. Buka `https://parking.yourdomain.com`
2. Cek SSL (gembok hijau)
3. Cek Chrome DevTools → Network → Headers ada `cf-ray`

---

## 📊 Cloudflare Analytics

Gratis di Cloudflare Dashboard:
- **Analytics** → Traffic, requests, bandwidth
- **Security** → Blocked threats
- **Speed** → Performance metrics

---

## 💡 Tips

1. **Purge Cache** setelah deploy:
   - Caching → Purge Everything

2. **Development Mode** saat testing:
   - Caching → Development Mode → On
   - (Cache dimatikan sementara)

3. **Under Attack Mode** jika kena DDoS:
   - Security → Under Attack Mode

---

## 🔄 Alternative: Gabungan

Kamu bisa gabung Cloudflare + EC2:

```
User → Cloudflare CDN → EC2 Frontend → EC2 Backend → RDS
                                            ↓
                                           S3
```

Ini setup paling hemat dengan fitur lengkap!
