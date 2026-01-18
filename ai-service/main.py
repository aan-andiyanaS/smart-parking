"""
Smart Parking AI Detection Service
Uses Ultralytics YOLO11 for parking slot detection

Features:
- Analyze parking images from ESP32-CAM
- Detect occupied/empty slots
- Update backend database via API
- Support for custom parking slot regions

Usage:
1. First run: python annotate_slots.py (to define parking regions)
2. Then run: python main.py (to start detection service)
"""

import os
import cv2
import json
import time
import requests
import numpy as np
from datetime import datetime
from flask import Flask, request, jsonify
from flask_cors import CORS
from ultralytics import YOLO
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Configuration
BACKEND_URL = os.getenv('BACKEND_URL', 'http://localhost')
YOLO_MODEL = os.getenv('YOLO_MODEL', 'yolo11n.pt')  # YOLOv11 nano
PARKING_REGIONS_FILE = 'parking_regions.json'
CONFIDENCE_THRESHOLD = 0.5
DETECTION_INTERVAL = 5  # seconds

# Flask app
app = Flask(__name__)
CORS(app)

# Global variables
model = None
parking_regions = []
last_detection = {
    'vehicles_detected': 0,
    'slot_status': {},
    'timestamp': None
}

def load_yolo_model():
    """Load YOLO model"""
    global model
    print(f"Loading YOLO model: {YOLO_MODEL}")
    try:
        model = YOLO(YOLO_MODEL)
        print("✅ YOLO model loaded successfully!")
        return True
    except Exception as e:
        print(f"❌ Failed to load YOLO model: {e}")
        return False

def load_parking_regions():
    """Load predefined parking slot regions from JSON"""
    global parking_regions
    if os.path.exists(PARKING_REGIONS_FILE):
        with open(PARKING_REGIONS_FILE, 'r') as f:
            parking_regions = json.load(f)
        print(f"✅ Loaded {len(parking_regions)} parking regions")
        return True
    else:
        print(f"⚠️ Parking regions file not found: {PARKING_REGIONS_FILE}")
        print("   Run: python annotate_slots.py to create parking regions")
        return False

def point_in_polygon(point, polygon):
    """Check if a point is inside a polygon using ray casting"""
    x, y = point
    n = len(polygon)
    inside = False
    
    p1x, p1y = polygon[0]
    for i in range(1, n + 1):
        p2x, p2y = polygon[i % n]
        if y > min(p1y, p2y):
            if y <= max(p1y, p2y):
                if x <= max(p1x, p2x):
                    if p1y != p2y:
                        xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                    if p1x == p2x or x <= xinters:
                        inside = not inside
        p1x, p1y = p2x, p2y
    
    return inside

def box_center(box):
    """Get center point of bounding box [x1, y1, x2, y2]"""
    x1, y1, x2, y2 = box
    return ((x1 + x2) / 2, (y1 + y2) / 2)

def detect_vehicles(image):
    """
    Detect vehicles in image using YOLO
    Returns list of detected vehicle bounding boxes
    """
    if model is None:
        print("Model not loaded!")
        return []
    
    # Run inference
    results = model(image, conf=CONFIDENCE_THRESHOLD, verbose=False)
    
    vehicles = []
    for result in results:
        boxes = result.boxes
        for box in boxes:
            cls = int(box.cls[0])
            conf = float(box.conf[0])
            
            # Class IDs for vehicles in COCO dataset:
            # 2: car, 3: motorcycle, 5: bus, 7: truck
            if cls in [2, 3, 5, 7]:
                xyxy = box.xyxy[0].tolist()
                vehicles.append({
                    'box': xyxy,
                    'class': cls,
                    'confidence': conf,
                    'center': box_center(xyxy)
                })
    
    return vehicles

def analyze_parking_slots(image, vehicles):
    """
    Analyze which parking slots are occupied based on detected vehicles
    Returns dict: {slot_code: is_occupied}
    """
    slot_status = {}
    
    for region in parking_regions:
        slot_code = region['code']
        polygon = region['points']
        
        # Check if any vehicle center is inside this parking slot
        is_occupied = False
        for vehicle in vehicles:
            if point_in_polygon(vehicle['center'], polygon):
                is_occupied = True
                break
        
        slot_status[slot_code] = is_occupied
    
    return slot_status

def analyze_image(image_path_or_array):
    """
    Main analysis function
    Takes image path or numpy array, returns slot occupancy
    """
    # Load image if path is provided
    if isinstance(image_path_or_array, str):
        image = cv2.imread(image_path_or_array)
    else:
        image = image_path_or_array
    
    if image is None:
        print("Failed to load image!")
        return None
    
    # Detect vehicles
    vehicles = detect_vehicles(image)
    print(f"Detected {len(vehicles)} vehicles")
    
    # Analyze slots
    if parking_regions:
        slot_status = analyze_parking_slots(image, vehicles)
        return {
            'vehicles_detected': len(vehicles),
            'slot_status': slot_status,
            'timestamp': datetime.now().isoformat()
        }
    else:
        # If no regions defined, just return vehicle count
        return {
            'vehicles_detected': len(vehicles),
            'slot_status': {},
            'timestamp': datetime.now().isoformat()
        }

def update_backend(slot_status):
    """Update slot status in backend via API"""
    try:
        # Get current slots from backend
        response = requests.get(f"{BACKEND_URL}/api/slots")
        if response.status_code != 200:
            print(f"Failed to fetch slots: {response.status_code}")
            return False
        
        slots = response.json().get('data', [])
        
        # Update each slot
        for slot in slots:
            slot_code = slot['code']
            if slot_code in slot_status:
                is_occupied = slot_status[slot_code]
                
                # Only update if status changed
                if slot['is_occupied'] != is_occupied:
                    update_response = requests.put(
                        f"{BACKEND_URL}/api/slots/{slot['id']}",
                        json={'is_occupied': is_occupied}
                    )
                    if update_response.status_code == 200:
                        print(f"Updated {slot_code}: {'Occupied' if is_occupied else 'Available'}")
                    else:
                        print(f"Failed to update {slot_code}")
        
        return True
    except Exception as e:
        print(f"Error updating backend: {e}")
        return False

def draw_results(image, vehicles, slot_status):
    """Draw detection results on image"""
    result_image = image.copy()
    
    # Draw parking regions
    for region in parking_regions:
        points = np.array(region['points'], dtype=np.int32)
        is_occupied = slot_status.get(region['code'], False)
        
        color = (0, 0, 255) if is_occupied else (0, 255, 0)  # Red if occupied, Green if empty
        cv2.polylines(result_image, [points], True, color, 2)
        
        # Draw label
        center = np.mean(points, axis=0).astype(int)
        status = "OCCUPIED" if is_occupied else "EMPTY"
        cv2.putText(result_image, f"{region['code']}: {status}", 
                   (center[0]-40, center[1]), cv2.FONT_HERSHEY_SIMPLEX, 
                   0.5, color, 2)
    
    # Draw vehicle boxes
    for vehicle in vehicles:
        x1, y1, x2, y2 = [int(x) for x in vehicle['box']]
        cv2.rectangle(result_image, (x1, y1), (x2, y2), (255, 0, 0), 2)
        cv2.putText(result_image, f"Vehicle {vehicle['confidence']:.2f}", 
                   (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)
    
    return result_image

# ==========================================
# Flask API Endpoints
# ==========================================

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({
        'status': 'ok',
        'model_loaded': model is not None,
        'regions_loaded': len(parking_regions) > 0
    })

@app.route('/analyze', methods=['POST'])
def analyze_endpoint():
    """
    Analyze uploaded image for parking occupancy
    
    POST /analyze
    Body: multipart/form-data with 'image' file
    
    Returns: {
        'success': true,
        'vehicles_detected': 2,
        'slot_status': {'P1': true, 'P2': false, ...}
    }
    """
    if 'image' not in request.files:
        return jsonify({'success': False, 'error': 'No image provided'}), 400
    
    file = request.files['image']
    
    # Read image
    npimg = np.frombuffer(file.read(), np.uint8)
    image = cv2.imdecode(npimg, cv2.IMREAD_COLOR)
    
    if image is None:
        return jsonify({'success': False, 'error': 'Invalid image'}), 400
    
    # Analyze
    result = analyze_image(image)
    
    if result is None:
        return jsonify({'success': False, 'error': 'Analysis failed'}), 500
    
    # Store for overlay endpoint
    global last_detection
    last_detection = {
        'vehicles_detected': result['vehicles_detected'],
        'slot_status': result['slot_status'],
        'timestamp': result['timestamp']
    }
    
    # Update backend
    if result['slot_status']:
        update_backend(result['slot_status'])
    
    return jsonify({
        'success': True,
        **result
    })

@app.route('/regions', methods=['GET'])
def get_regions():
    """Get defined parking regions"""
    return jsonify({
        'success': True,
        'regions': parking_regions
    })

@app.route('/regions', methods=['POST'])
def set_regions():
    """Set parking regions"""
    global parking_regions
    
    data = request.json
    if not data or 'regions' not in data:
        return jsonify({'success': False, 'error': 'No regions provided'}), 400
    
    parking_regions = data['regions']
    
    # Save to file
    with open(PARKING_REGIONS_FILE, 'w') as f:
        json.dump(parking_regions, f, indent=2)
    
    return jsonify({
        'success': True,
        'message': f'Saved {len(parking_regions)} regions'
    })

@app.route('/overlay', methods=['GET'])
def get_overlay():
    """
    Get overlay data for frontend rendering
    
    Returns parking regions (polygons) with current slot status
    Frontend can use this to draw colored overlays on live video
    
    GET /overlay
    
    Returns: {
        'success': true,
        'regions': [
            {
                'code': 'P1',
                'points': [[x1,y1], [x2,y2], ...],
                'is_occupied': true/false
            },
            ...
        ],
        'last_detection': {
            'vehicles_detected': 2,
            'timestamp': '2026-01-17T22:00:00'
        }
    }
    """
    # Combine regions with status
    overlay_regions = []
    for region in parking_regions:
        overlay_regions.append({
            'code': region['code'],
            'points': region['points'],
            'is_occupied': last_detection['slot_status'].get(region['code'], False)
        })
    
    return jsonify({
        'success': True,
        'regions': overlay_regions,
        'last_detection': {
            'vehicles_detected': last_detection['vehicles_detected'],
            'timestamp': last_detection['timestamp']
        }
    })

# ==========================================
# Main
# ==========================================

if __name__ == '__main__':
    print("=" * 50)
    print("Smart Parking AI Detection Service")
    print("=" * 50)
    
    # Load model
    if not load_yolo_model():
        print("Warning: YOLO model not loaded. Detection will not work.")
    
    # Load parking regions
    load_parking_regions()
    
    # Start Flask server
    port = int(os.getenv('PORT', 5000))
    print(f"\n🚀 Starting AI service on port {port}")
    print(f"   Endpoints:")
    print(f"   - GET  /health   - Health check")
    print(f"   - POST /analyze  - Analyze parking image")
    print(f"   - GET  /regions  - Get parking regions")
    print(f"   - POST /regions  - Set parking regions")
    
    app.run(host='0.0.0.0', port=port, debug=True)
