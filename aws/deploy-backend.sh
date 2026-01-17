#!/bin/bash
# Deploy Backend to EC2
# Run this script on your EC2 Backend instance

set -e

echo "==================================="
echo "Smart Parking - Backend Deployment"
echo "==================================="

# Update system
sudo apt-get update
sudo apt-get upgrade -y

# Install Docker
if ! command -v docker &> /dev/null; then
    echo "Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sudo sh get-docker.sh
    sudo usermod -aG docker $USER
    rm get-docker.sh
fi

# Install Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo "Installing Docker Compose..."
    sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
fi

# Clone or pull repo
REPO_URL="https://github.com/YOUR_USERNAME/smart-parking.git"
APP_DIR="/home/ubuntu/smart-parking"

if [ -d "$APP_DIR" ]; then
    echo "Pulling latest changes..."
    cd $APP_DIR
    git pull origin main
else
    echo "Cloning repository..."
    git clone $REPO_URL $APP_DIR
    cd $APP_DIR
fi

# Copy environment file
if [ ! -f ".env" ]; then
    echo "Creating .env file..."
    cp aws/.env.backend.example .env
    echo "⚠️  Please edit .env file with your AWS credentials!"
    nano .env
fi

# Build and run
echo "Building and starting containers..."
cd $APP_DIR
docker-compose -f aws/backend.docker-compose.yml up -d --build

# Show status
echo ""
echo "==================================="
echo "Deployment Complete!"
echo "==================================="
docker ps

echo ""
echo "Backend API: http://$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4):8080"
