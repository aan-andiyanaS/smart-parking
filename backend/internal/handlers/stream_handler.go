package handlers

import (
	"io"
	"log"
	"net/http"
	"os"
	"time"

	"github.com/gin-gonic/gin"
)

// StreamHandler handles camera stream proxying
type StreamHandler struct {
	esp32URL string
}

// NewStreamHandler creates a new StreamHandler
func NewStreamHandler() *StreamHandler {
	esp32URL := os.Getenv("ESP32_STREAM_URL")
	if esp32URL == "" {
		esp32URL = "http://192.168.1.50" // Default ESP32 IP
	}
	return &StreamHandler{esp32URL: esp32URL}
}

// ProxyStream proxies MJPEG stream from ESP32 to client
func (h *StreamHandler) ProxyStream(c *gin.Context) {
	streamURL := h.esp32URL + "/stream"

	log.Printf("Proxying stream from: %s", streamURL)

	// Create HTTP client with timeout
	client := &http.Client{
		Timeout: 0, // No timeout for streaming
	}

	// Create request to ESP32
	req, err := http.NewRequest("GET", streamURL, nil)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to create stream request",
		})
		return
	}

	// Set request context to cancel when client disconnects
	req = req.WithContext(c.Request.Context())

	// Make request to ESP32
	resp, err := client.Do(req)
	if err != nil {
		log.Printf("Stream error: %v", err)
		c.JSON(http.StatusBadGateway, gin.H{
			"success": false,
			"error":   "Failed to connect to camera: " + err.Error(),
		})
		return
	}
	defer resp.Body.Close()

	// Set headers for MJPEG stream
	c.Header("Content-Type", resp.Header.Get("Content-Type"))
	c.Header("Cache-Control", "no-cache, no-store, must-revalidate")
	c.Header("Pragma", "no-cache")
	c.Header("Expires", "0")
	c.Header("Access-Control-Allow-Origin", "*")

	// Stream the response
	c.Status(http.StatusOK)

	// Use io.Copy to efficiently stream data
	_, err = io.Copy(c.Writer, resp.Body)
	if err != nil {
		log.Printf("Stream copy ended: %v", err)
	}
}

// ProxyCapture proxies single frame capture from ESP32
func (h *StreamHandler) ProxyCapture(c *gin.Context) {
	captureURL := h.esp32URL + "/capture"

	log.Printf("Proxying capture from: %s", captureURL)

	client := &http.Client{
		Timeout: 10 * time.Second,
	}

	resp, err := client.Get(captureURL)
	if err != nil {
		c.JSON(http.StatusBadGateway, gin.H{
			"success": false,
			"error":   "Failed to capture from camera",
		})
		return
	}
	defer resp.Body.Close()

	// Read image data
	imageData, err := io.ReadAll(resp.Body)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to read capture data",
		})
		return
	}

	// Send image
	c.Header("Access-Control-Allow-Origin", "*")
	c.Data(http.StatusOK, "image/jpeg", imageData)
}

// GetCameraStatus returns ESP32 camera status
func (h *StreamHandler) GetCameraStatus(c *gin.Context) {
	statusURL := h.esp32URL + "/status"

	client := &http.Client{
		Timeout: 5 * time.Second,
	}

	resp, err := client.Get(statusURL)
	if err != nil {
		c.JSON(http.StatusOK, gin.H{
			"success":   false,
			"connected": false,
			"error":     "Camera not reachable",
		})
		return
	}
	defer resp.Body.Close()

	// Read and forward status
	body, _ := io.ReadAll(resp.Body)

	c.Header("Access-Control-Allow-Origin", "*")
	c.Data(http.StatusOK, "application/json", body)
}

// SetCameraURL sets the ESP32 camera URL
func (h *StreamHandler) SetCameraURL(c *gin.Context) {
	var req struct {
		URL string `json:"url" binding:"required"`
	}

	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "URL is required",
		})
		return
	}

	h.esp32URL = req.URL
	log.Printf("Camera URL updated to: %s", h.esp32URL)

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"message": "Camera URL updated",
		"url":     h.esp32URL,
	})
}
