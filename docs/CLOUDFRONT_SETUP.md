# CloudFront CDN Setup Guide

Panduan lengkap untuk setup CloudFront sebagai CDN untuk Smart Parking System.

---

## 📋 Apa itu CloudFront?

CloudFront adalah CDN (Content Delivery Network) dari AWS yang:
- ✅ Mempercepat loading website
- ✅ Memberikan HTTPS gratis
- ✅ Cache static files di edge locations
- ✅ Melindungi dari DDoS (AWS Shield)

---

## 🚀 Step-by-Step Setup

### Step 1: Buka CloudFront Console

1. Login ke AWS Console
2. **Services → CloudFront**
3. Klik **Create distribution**

---

### Step 2: Configure Origin

**Origin Settings:**

| Field | Value | Keterangan |
|-------|-------|------------|
| Origin domain | `ec2-frontend-ip.ap-southeast-1.compute.amazonaws.com` | EC2 Frontend Public DNS |
| Protocol | HTTP only | Nginx di EC2 pakai HTTP |
| HTTP port | 80 | Default |
| Origin path | (kosong) | - |
| Name | `frontend-origin` | Nama origin |

---

### Step 3: Configure Cache Behavior

**Default cache behavior:**

| Field | Value |
|-------|-------|
| Path pattern | `Default (*)` |
| Compress objects automatically | **Yes** |
| Viewer protocol policy | **Redirect HTTP to HTTPS** |
| Allowed HTTP methods | **GET, HEAD, OPTIONS, PUT, POST, PATCH, DELETE** |
| Cache policy | **CachingOptimized** |
| Origin request policy | **AllViewer** |

---

### Step 4: Configure Settings

**Settings:**

| Field | Value |
|-------|-------|
| Price class | **Use only North America and Europe** (termurah) |
| AWS WAF | None (skip untuk sekarang) |
| Alternate domain name (CNAME) | (kosong, nanti bisa ditambah) |
| Custom SSL certificate | Default CloudFront certificate |
| Default root object | `index.html` |
| Standard logging | Off |

---

### Step 5: Create Distribution

1. Klik **Create distribution**
2. Tunggu 5-15 menit sampai status **Deployed**
3. Catat **Distribution domain name**: `d1234abcd.cloudfront.net`

---

### Step 6: Configure Error Pages (Untuk SPA)

React adalah Single Page App, perlu handle 404:

1. Klik distribution → **Error pages** tab
2. Klik **Create custom error response**

| Field | Value |
|-------|-------|
| HTTP error code | 403 |
| Customize error response | Yes |
| Response page path | `/index.html` |
| HTTP response code | 200 |

3. Buat lagi untuk error code **404** dengan setting sama

---

### Step 7: Test

Buka browser:
```
https://d1234abcd.cloudfront.net
```

Harus muncul dashboard Smart Parking! 🎉

---

## 🔄 Invalidate Cache

Setelah update frontend, perlu clear cache:

**Via AWS Console:**
1. CloudFront → Distribution → **Invalidations** tab
2. Create invalidation
3. Object paths: `/*`
4. Create

**Via AWS CLI:**
```bash
aws cloudfront create-invalidation \
  --distribution-id YOUR_DISTRIBUTION_ID \
  --paths "/*"
```

---

## 🔒 Optional: Custom Domain + SSL

Jika punya domain sendiri (misal: `parking.example.com`):

### 1. Request SSL Certificate (ACM)
1. **Services → Certificate Manager**
2. Region: **US East (N. Virginia)** ← Wajib untuk CloudFront!
3. Request certificate → Public certificate
4. Domain: `parking.example.com`
5. Validation: DNS validation
6. Tambah CNAME record di DNS

### 2. Update CloudFront
1. Edit distribution
2. Alternate domain name: `parking.example.com`
3. Custom SSL certificate: Pilih certificate dari ACM
4. Save

### 3. Update DNS
Di domain registrar, tambah:
```
Type: CNAME
Name: parking
Value: d1234abcd.cloudfront.net
```

---

## 💰 Biaya CloudFront

| Item | Free Tier | Setelah Free Tier |
|------|-----------|-------------------|
| Data Transfer | 1TB/bulan (1 tahun) | $0.085/GB |
| Requests | 10M/bulan (1 tahun) | $0.0075/10K |

Untuk project demo, biasanya **masih free** atau di bawah $5/bulan.
