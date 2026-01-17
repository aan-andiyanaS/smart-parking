#!/bin/bash
# Deploy Frontend to EC2
# Run this script on your EC2 Frontend instance

set -e

echo "===================================="
echo "Smart Parking - Frontend Deployment"
echo "===================================="

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

# Install Node.js (for building frontend)
if ! command -v node &> /dev/null; then
    echo "Installing Node.js..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
    sudo apt-get install -y nodejs
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

# Build frontend
echo "Building frontend..."
cd $APP_DIR/frontend
npm install
npm run build

# Update nginx config with backend IP
BACKEND_IP="YOUR_BACKEND_EC2_PRIVATE_IP"  # Change this!
sed -i "s/BACKEND_EC2_PRIVATE_IP/$BACKEND_IP/g" $APP_DIR/nginx/frontend.nginx.conf

# Run with Docker Compose
echo "Starting frontend container..."
cd $APP_DIR
docker-compose -f aws/frontend.docker-compose.yml up -d --build

# Show status
echo ""
echo "===================================="
echo "Deployment Complete!"
echo "===================================="
docker ps

PUBLIC_IP=$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)
echo ""
echo "Frontend URL: http://$PUBLIC_IP"
echo ""
echo "Next steps:"
echo "1. Configure CloudFront to point to this EC2"
echo "2. Setup SSL certificate with Let's Encrypt"
