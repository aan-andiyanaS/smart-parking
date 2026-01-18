#!/bin/bash
# ===========================================
# Smart Parking - AI Service Starter (Linux/Mac)
# ===========================================

echo ""
echo "========================================"
echo "  Smart Parking AI Service"
echo "========================================"
echo ""

# Navigate to script directory
cd "$(dirname "$0")/ai-service"

# Check if conda is available
if ! command -v conda &> /dev/null; then
    echo "[ERROR] Conda not found in PATH"
    echo "Please install Anaconda/Miniconda first"
    exit 1
fi

# Activate conda environment
echo "[INFO] Activating conda environment..."
source "$(conda info --base)/etc/profile.d/conda.sh"
conda activate smartparking 2>/dev/null

if [ $? -ne 0 ]; then
    echo "[WARN] Environment 'smartparking' not found"
    echo "[INFO] Creating new environment..."
    conda create -n smartparking python=3.10 -y
    conda activate smartparking
fi

# Install dependencies
echo "[INFO] Installing dependencies..."
pip install -r requirements.txt -q

# Run AI service
echo ""
echo "[INFO] Starting AI Service on port 5000..."
echo "[INFO] Press Ctrl+C to stop"
echo ""
python main.py
