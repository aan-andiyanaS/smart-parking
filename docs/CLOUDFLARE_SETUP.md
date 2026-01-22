# Cloudflare Setup Guide

Panduan setup Cloudflare untuk Smart Parking System dengan arsitektur hybrid (Cloudflare Pages + AWS).

---

## 💰 Kenapa Cloudflare?

| Fitur | AWS CloudFront | Cloudflare |
|-------|----------------|------------|
| CDN | ✅ | ✅ |
| SSL Gratis | ✅ | ✅ |
| DDoS Protection | Bayar | **GRATIS** |
| WAF | Bayar | **GRATIS (Basic)** |
| Static Hosting | ❌ (butuh S3) | ✅ **Pages (Gratis)** |
| Auto Deploy | ❌ | ✅ **dari GitHub** |
| Harga | ~$5-10/bulan | **$0** |

---

## 📋 Arsitektur dengan Cloudflare

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              CLOUDFLARE                                       │
│                                                                               │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                      CLOUDFLARE PAGES                                   │  │
│  │                                                                         │  │
│  │  • parking.yourdomain.com                                              │  │
│  │  • Auto deploy dari GitHub                                             │  │
│  │  • Global CDN (200+ lokasi)                                            │  │
│  │  • SSL otomatis                                                        │  │
│  │                                                                         │  │
│  │  Hosts: React Dashboard (Static HTML/JS/CSS)                           │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                                                               │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                      CLOUDFLARE PROXY                                   │  │
│  │                                                                         │  │
│  │  • api.yourdomain.com → EC2 Backend (54.xxx.xxx.xxx)                   │  │
│  │  • DDoS Protection                                                     │  │
│  │  • WAF Basic (free)                                                    │  │
│  │  • SSL Termination                                                     │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
                              AWS EC2 Backend
```

---

## 🚀 Setup Langkah demi Langkah

### Step 1: Daftar Cloudflare

1. Buka https://dash.cloudflare.com/sign-up
2. Daftar dengan email
3. Verify email

---

### Step 2: Add Domain

1. Klik **Add site**
2. Masukkan domain: `yourdomain.com`
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
4. Tunggu propagasi (biasanya <1 jam)

---

### Step 4: Setup Cloudflare Pages (Frontend)

1. **Dashboard → Pages → Create a project**
2. **Connect to Git → GitHub**
3. Authorize dan pilih repository `smart-parking`
4. Configure build:

| Setting | Value |
|---------|-------|
| Project name | `smart-parking` |
| Production branch | `main` |
| Framework preset | `Vite` |
| Build command | `npm run build` |
| Build output directory | `dist` |
| Root directory | `frontend` |

5. **Environment variables** (⚠️ PENTING):
   - `VITE_API_URL` = `https://api.yourdomain.com`
   - `VITE_WS_URL` = `wss://api.yourdomain.com/ws`

6. **Save and Deploy**

---

### Step 5: Add DNS Records

Di Cloudflare Dashboard → **DNS** → **Records** → **Add record**

| Type | Name | Content | Proxy |
|------|------|---------|-------|
| CNAME | `parking` | `smart-parking.pages.dev` | 🟠 Proxied |
| A | `api` | `54.xxx.xxx.xxx` (EC2 IP) | 🟠 Proxied |

**Hasil:**
- `parking.yourdomain.com` → Cloudflare Pages (Frontend)
- `api.yourdomain.com` → EC2 Backend (via Cloudflare Proxy)

---

### Step 6: Custom Domain untuk Pages

1. **Pages → smart-parking → Custom domains**
2. **Set up a custom domain**
3. Masukkan: `parking.yourdomain.com`
4. Cloudflare akan auto-configure DNS

---

### Step 7: Enable SSL

1. **SSL/TLS** → **Overview**
2. Mode: **Full (strict)** jika EC2 punya SSL, atau **Flexible** jika tidak

---

## 🔧 Environment Variables

### Di Cloudflare Pages

1. **Pages → smart-parking → Settings → Environment variables**
2. Add:
   - `VITE_API_URL` = `https://api.yourdomain.com`
   - `VITE_WS_URL` = `wss://api.yourdomain.com/ws`

---

## ✅ Testing

1. Buka `https://parking.yourdomain.com`
2. Cek SSL (gembok hijau) ✅
3. Chrome DevTools → Network → Headers → cari `cf-ray` ✅
4. Test API: `https://api.yourdomain.com/api/health` ✅

---

## 📊 Cloudflare Analytics (Gratis!)

- **Analytics** → Traffic, requests, bandwidth
- **Security** → Blocked threats, firewall events
- **Speed** → Performance metrics

---

## 💡 Tips

### Development Mode
Saat testing, disable cache sementara:
- **Caching → Development Mode → On**

### Purge Cache
Setelah deploy baru (tidak perlu untuk Pages, otomatis):
- **Caching → Purge Everything**

### Under Attack Mode
Jika kena DDoS:
- **Security → Under Attack Mode**

---

## ❌ Troubleshooting

### Pages Deploy Failed
- Cek **Pages → Deployments → View logs**
- Pastikan build command benar: `npm run build`
- Pastikan environment variables sudah di-set

### API Not Accessible
- Cek DNS record untuk `api.yourdomain.com`
- Cek EC2 Security Group allow port 8080 dari Cloudflare IPs
- Test: `curl https://api.yourdomain.com/api/health`

### CORS Error
- Pastikan backend allow origin dari `https://parking.yourdomain.com`
