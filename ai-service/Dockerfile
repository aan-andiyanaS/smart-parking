FROM python:3.11-slim

WORKDIR /app

# Install system dependencies for OpenCV
RUN apt-get update && apt-get install -y \
    libgl1-mesa-glx \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxrender-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements first for caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Download YOLO model during build
RUN python -c "from ultralytics import YOLO; YOLO('yolo11n.pt')"

# Copy application code
COPY . .

# Expose port
EXPOSE 5000

# Run the service
CMD ["python", "main.py"]
