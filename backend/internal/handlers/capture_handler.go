package handlers

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"mime/multipart"
	"net/http"
	"os"
	"path/filepath"
	"time"

	"smart-parking/internal/models"
	"smart-parking/internal/services"
	"smart-parking/pkg/websocket"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"
)

// CaptureHandler handles capture-related HTTP requests
type CaptureHandler struct {
	db      *gorm.DB
	storage *services.MinIOStorage
	hub     *websocket.Hub
}

// NewCaptureHandler creates a new CaptureHandler
func NewCaptureHandler(db *gorm.DB, storage *services.MinIOStorage, hub *websocket.Hub) *CaptureHandler {
	return &CaptureHandler{db: db, storage: storage, hub: hub}
}

// AIAnalysisResponse represents the response from AI service
type AIAnalysisResponse struct {
	Success          bool            `json:"success"`
	VehiclesDetected int             `json:"vehicles_detected"`
	SlotStatus       map[string]bool `json:"slot_status"`
	Timestamp        string          `json:"timestamp"`
}

// Upload handles image upload from ESP32-CAM or web
func (h *CaptureHandler) Upload(c *gin.Context) {
	// Get file from request
	file, header, err := c.Request.FormFile("image")
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "No image file provided",
		})
		return
	}
	defer file.Close()

	// Read file content for reuse
	fileBytes, err := io.ReadAll(file)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to read image file",
		})
		return
	}

	// Get camera ID (optional)
	cameraID := c.PostForm("camera_id")
	if cameraID == "" {
		cameraID = "cam1"
	}

	// Check storage availability
	if h.storage == nil {
		c.JSON(http.StatusServiceUnavailable, gin.H{
			"success": false,
			"error":   "Storage service not available",
		})
		return
	}

	// Generate filename
	ext := filepath.Ext(header.Filename)
	if ext == "" {
		ext = ".jpg"
	}
	filename := fmt.Sprintf("captures/%s/%s_%s%s",
		time.Now().Format("2006/01/02"),
		cameraID,
		uuid.New().String()[:8],
		ext,
	)

	// Upload to MinIO
	imageURL, err := h.storage.Upload(c.Request.Context(), bytes.NewReader(fileBytes), filename, int64(len(fileBytes)), header.Header.Get("Content-Type"))
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   fmt.Sprintf("Failed to upload image: %v", err),
		})
		return
	}

	// Save capture record to database
	capture := models.Capture{
		ImageURL: imageURL,
		CameraID: cameraID,
	}

	if err := h.db.Create(&capture).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to save capture record",
		})
		return
	}

	// Broadcast new capture via WebSocket
	h.hub.Broadcast(websocket.Message{
		Type: "new_capture",
		Data: capture,
	})

	// Call AI service for auto slot detection (async, don't block response)
	go h.analyzeWithAI(fileBytes, header.Filename)

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    capture,
	})
}

// analyzeWithAI sends image to AI service for slot detection
func (h *CaptureHandler) analyzeWithAI(imageData []byte, filename string) {
	aiServiceURL := os.Getenv("AI_SERVICE_URL")
	if aiServiceURL == "" {
		aiServiceURL = "http://localhost:5000"
	}

	// Create multipart form
	body := &bytes.Buffer{}
	writer := multipart.NewWriter(body)
	
	part, err := writer.CreateFormFile("image", filename)
	if err != nil {
		log.Printf("AI Analysis: Failed to create form file: %v", err)
		return
	}
	
	part.Write(imageData)
	writer.Close()

	// Send to AI service
	resp, err := http.Post(aiServiceURL+"/analyze", writer.FormDataContentType(), body)
	if err != nil {
		log.Printf("AI Analysis: Failed to call AI service: %v", err)
		return
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		log.Printf("AI Analysis: AI service returned status %d", resp.StatusCode)
		return
	}

	// Parse response
	var aiResponse AIAnalysisResponse
	if err := json.NewDecoder(resp.Body).Decode(&aiResponse); err != nil {
		log.Printf("AI Analysis: Failed to parse response: %v", err)
		return
	}

	log.Printf("AI Analysis: Detected %d vehicles", aiResponse.VehiclesDetected)

	// Update slot status in database based on AI detection
	for slotCode, isOccupied := range aiResponse.SlotStatus {
		var slot models.Slot
		if err := h.db.Where("code = ?", slotCode).First(&slot).Error; err != nil {
			continue
		}

		// Only update if status changed
		if slot.IsOccupied != isOccupied {
			slot.IsOccupied = isOccupied
			h.db.Save(&slot)
			
			// Broadcast update via WebSocket
			h.hub.Broadcast(websocket.Message{
				Type: "slot_update",
				Data: slot,
			})
			
			log.Printf("AI Analysis: Updated %s to %v", slotCode, isOccupied)
		}
	}
}

// GetLatest returns the latest capture
func (h *CaptureHandler) GetLatest(c *gin.Context) {
	var capture models.Capture

	if err := h.db.Order("captured_at DESC").First(&capture).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{
			"success": false,
			"error":   "No captures found",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    capture,
	})
}

// GetAll returns all captures with pagination
func (h *CaptureHandler) GetAll(c *gin.Context) {
	var captures []models.Capture

	// Limit to last 50 captures
	if err := h.db.Order("captured_at DESC").Limit(50).Find(&captures).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to fetch captures",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    captures,
		"total":   len(captures),
	})
}
