"""
Parking Slot Annotation Tool
Use this to define parking slot regions on your parking lot image

Usage:
1. Run: python annotate_slots.py
2. Click on the image to define polygon corners for each parking slot
3. Press 'n' for next slot, 's' to save, 'r' to reset current, 'q' to quit
"""

import cv2
import json
import os
import sys

# Configuration
OUTPUT_FILE = 'parking_regions.json'
SAMPLE_IMAGE = 'sample_parking.jpg'

# State
current_points = []
all_regions = []
current_slot_index = 1
image = None
original_image = None

def mouse_callback(event, x, y, flags, param):
    """Handle mouse clicks to define polygon points"""
    global current_points, image
    
    if event == cv2.EVENT_LBUTTONDOWN:
        current_points.append([x, y])
        
        # Draw point
        cv2.circle(image, (x, y), 5, (0, 255, 0), -1)
        
        # Draw lines connecting points
        if len(current_points) > 1:
            cv2.line(image, tuple(current_points[-2]), tuple(current_points[-1]), (0, 255, 0), 2)
        
        # Close polygon preview if 4+ points
        if len(current_points) >= 4:
            cv2.line(image, tuple(current_points[-1]), tuple(current_points[0]), (0, 255, 0), 2)
        
        cv2.imshow('Parking Slot Annotator', image)

def draw_existing_regions():
    """Draw all saved regions on image"""
    global image
    
    for region in all_regions:
        points = region['points']
        color = (255, 0, 0)  # Blue for saved regions
        
        for i in range(len(points)):
            cv2.line(image, tuple(points[i]), tuple(points[(i+1) % len(points)]), color, 2)
        
        # Draw label
        center_x = sum(p[0] for p in points) // len(points)
        center_y = sum(p[1] for p in points) // len(points)
        cv2.putText(image, region['code'], (center_x-15, center_y), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

def main():
    global current_points, all_regions, current_slot_index, image, original_image
    
    print("=" * 50)
    print("Parking Slot Annotation Tool")
    print("=" * 50)
    
    # Check for sample image
    if len(sys.argv) > 1:
        image_path = sys.argv[1]
    elif os.path.exists(SAMPLE_IMAGE):
        image_path = SAMPLE_IMAGE
    else:
        print(f"\nNo image found. Please provide an image:")
        print(f"  python annotate_slots.py <image_path>")
        print(f"\nOr place a '{SAMPLE_IMAGE}' in this directory.")
        
        # Create sample image for demo
        print("\nCreating sample image for demo...")
        sample = create_sample_image()
        cv2.imwrite(SAMPLE_IMAGE, sample)
        image_path = SAMPLE_IMAGE
        print(f"Created {SAMPLE_IMAGE}")
    
    # Load image
    original_image = cv2.imread(image_path)
    if original_image is None:
        print(f"Error: Could not load image: {image_path}")
        return
    
    image = original_image.copy()
    
    # Load existing regions if any
    if os.path.exists(OUTPUT_FILE):
        with open(OUTPUT_FILE, 'r') as f:
            all_regions = json.load(f)
        current_slot_index = len(all_regions) + 1
        draw_existing_regions()
        print(f"Loaded {len(all_regions)} existing regions")
    
    print(f"\nImage loaded: {image_path}")
    print(f"Image size: {original_image.shape[1]}x{original_image.shape[0]}")
    print()
    print("Instructions:")
    print("  - Left click: Add point to current polygon")
    print("  - 'n': Save current slot and start next")
    print("  - 'r': Reset current slot points")
    print("  - 's': Save all regions to file")
    print("  - 'u': Undo last saved slot")
    print("  - 'q': Quit")
    print()
    
    # Create window and set callback
    cv2.namedWindow('Parking Slot Annotator')
    cv2.setMouseCallback('Parking Slot Annotator', mouse_callback)
    
    # Update title
    update_title()
    
    while True:
        cv2.imshow('Parking Slot Annotator', image)
        key = cv2.waitKey(1) & 0xFF
        
        if key == ord('q'):
            break
        
        elif key == ord('n'):
            # Save current slot and start next
            if len(current_points) >= 3:
                slot_code = f"P{current_slot_index}"
                all_regions.append({
                    'code': slot_code,
                    'points': current_points.copy()
                })
                print(f"Saved slot {slot_code} with {len(current_points)} points")
                current_slot_index += 1
                current_points = []
                
                # Redraw
                image = original_image.copy()
                draw_existing_regions()
                update_title()
            else:
                print("Need at least 3 points to define a slot!")
        
        elif key == ord('r'):
            # Reset current slot
            current_points = []
            image = original_image.copy()
            draw_existing_regions()
            print("Reset current slot points")
        
        elif key == ord('s'):
            # Save to file
            with open(OUTPUT_FILE, 'w') as f:
                json.dump(all_regions, f, indent=2)
            print(f"✅ Saved {len(all_regions)} regions to {OUTPUT_FILE}")
        
        elif key == ord('u'):
            # Undo last slot
            if all_regions:
                removed = all_regions.pop()
                current_slot_index -= 1
                print(f"Removed slot {removed['code']}")
                image = original_image.copy()
                draw_existing_regions()
                update_title()
    
    cv2.destroyAllWindows()
    
    # Final save
    if all_regions:
        with open(OUTPUT_FILE, 'w') as f:
            json.dump(all_regions, f, indent=2)
        print(f"\n✅ Final save: {len(all_regions)} regions to {OUTPUT_FILE}")

def update_title():
    """Update window title with current slot info"""
    title = f"Parking Slot Annotator - Defining P{current_slot_index} | Total: {len(all_regions)} slots"
    cv2.setWindowTitle('Parking Slot Annotator', title)

def create_sample_image():
    """Create a sample parking lot image for demo"""
    # Create 800x600 gray image
    img = 170 * cv2.resize(cv2.imread(cv2.samples.findFile('lena.jpg')) if cv2.samples.findFile('lena.jpg') else 
                           (170 * np.ones((600, 800, 3), dtype=np.uint8)), (800, 600))
    img = 170 * np.ones((600, 800, 3), dtype=np.uint8)
    
    # Draw parking lines
    for i in range(5):
        x = 100 + i * 150
        cv2.rectangle(img, (x, 100), (x+120, 280), (255, 255, 255), 2)
        cv2.putText(img, f'P{i+1}', (x+40, 200), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    
    for i in range(5):
        x = 100 + i * 150
        cv2.rectangle(img, (x, 320), (x+120, 500), (255, 255, 255), 2)
        cv2.putText(img, f'P{i+6}', (x+40, 420), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    
    return img

import numpy as np

if __name__ == '__main__':
    main()
