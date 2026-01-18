@echo off
REM ===========================================
REM Smart Parking - AI Service Starter (Windows)
REM ===========================================

echo.
echo ========================================
echo   Smart Parking AI Service
echo ========================================
echo.

REM Check if conda is available
where conda >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Conda not found in PATH
    echo Please install Anaconda/Miniconda or activate conda first
    pause
    exit /b 1
)

REM Navigate to ai-service folder
cd /d "%~dp0ai-service"

REM Activate conda environment (adjust name if needed)
echo [INFO] Activating conda environment...
call conda activate smartparking 2>nul
if %errorlevel% neq 0 (
    echo [WARN] Environment 'smartparking' not found
    echo [INFO] Creating new environment...
    call conda create -n smartparking python=3.10 -y
    call conda activate smartparking
)

REM Install dependencies
echo [INFO] Installing dependencies...
pip install -r requirements.txt -q

REM Run AI service
echo.
echo [INFO] Starting AI Service on port 5000...
echo [INFO] Press Ctrl+C to stop
echo.
python main.py
