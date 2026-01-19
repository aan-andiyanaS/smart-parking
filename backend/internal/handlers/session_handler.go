package handlers

import (
	"net/http"
	"time"

	"smart-parking/internal/models"
	"smart-parking/pkg/websocket"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

// SessionHandler handles parking session-related HTTP requests
type SessionHandler struct {
	db  *gorm.DB
	hub *websocket.Hub
}

// NewSessionHandler creates a new SessionHandler
func NewSessionHandler(db *gorm.DB, hub *websocket.Hub) *SessionHandler {
	return &SessionHandler{db: db, hub: hub}
}

// EntryRequest represents the entry event request from ESP32
type EntryRequest struct {
	CameraID  string `json:"camera_id"`
	EventType string `json:"event_type"`
}

// ExitRequest represents the exit event request from ESP32
type ExitRequest struct {
	CameraID  string `json:"camera_id"`
	EventType string `json:"event_type"`
}

// HandleEntry creates a new parking session when a vehicle enters
func (h *SessionHandler) HandleEntry(c *gin.Context) {
	var req EntryRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "Invalid request body",
		})
		return
	}

	// Create new session with status 'active'
	session := models.Session{
		Status: "active",
	}

	if err := h.db.Create(&session).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to create session",
		})
		return
	}

	// Broadcast session event via WebSocket
	h.hub.Broadcast(websocket.Message{
		Type: "session_entry",
		Data: gin.H{
			"session_id": session.ID,
			"entry_time": session.EntryTime,
			"camera_id":  req.CameraID,
		},
	})

	c.JSON(http.StatusCreated, gin.H{
		"success": true,
		"message": "Entry recorded",
		"data":    session,
	})
}

// HandleExit closes the most recent active session when a vehicle exits
func (h *SessionHandler) HandleExit(c *gin.Context) {
	var req ExitRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "Invalid request body",
		})
		return
	}

	// Find the oldest active session (FIFO)
	var session models.Session
	if err := h.db.Where("status = ?", "active").Order("entry_time ASC").First(&session).Error; err != nil {
		if err == gorm.ErrRecordNotFound {
			c.JSON(http.StatusNotFound, gin.H{
				"success": false,
				"error":   "No active session found",
			})
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to find active session",
		})
		return
	}

	// Update session with exit time and calculate duration
	now := time.Now()
	session.ExitTime = &now
	duration := int(now.Sub(session.EntryTime).Minutes())
	session.DurationMinutes = &duration
	session.Status = "completed"

	// Calculate fee (example: Rp 2000 per hour, minimum Rp 2000)
	feePerHour := 2000.0
	hours := float64(duration) / 60.0
	if hours < 1 {
		hours = 1 // Minimum 1 hour
	}
	fee := hours * feePerHour
	session.TotalFee = &fee

	if err := h.db.Save(&session).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to update session",
		})
		return
	}

	// Broadcast session event via WebSocket
	h.hub.Broadcast(websocket.Message{
		Type: "session_exit",
		Data: gin.H{
			"session_id":       session.ID,
			"entry_time":       session.EntryTime,
			"exit_time":        session.ExitTime,
			"duration_minutes": session.DurationMinutes,
			"total_fee":        session.TotalFee,
			"camera_id":        req.CameraID,
		},
	})

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"message": "Exit recorded",
		"data":    session,
	})
}

// GetAll returns all parking sessions
func (h *SessionHandler) GetAll(c *gin.Context) {
	var sessions []models.Session

	// Get status filter from query param
	status := c.Query("status")

	query := h.db.Order("created_at DESC")
	if status != "" {
		query = query.Where("status = ?", status)
	}

	if err := query.Limit(100).Find(&sessions).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to fetch sessions",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    sessions,
	})
}

// GetByID returns a single session by ID
func (h *SessionHandler) GetByID(c *gin.Context) {
	id := c.Param("id")

	var session models.Session
	if err := h.db.First(&session, "id = ?", id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{
			"success": false,
			"error":   "Session not found",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    session,
	})
}

// GetStats returns session statistics
func (h *SessionHandler) GetStats(c *gin.Context) {
	var activeCount, completedCount, todayCount int64

	// Count active sessions
	h.db.Model(&models.Session{}).Where("status = ?", "active").Count(&activeCount)

	// Count completed sessions
	h.db.Model(&models.Session{}).Where("status = ?", "completed").Count(&completedCount)

	// Count today's sessions
	today := time.Now().Truncate(24 * time.Hour)
	h.db.Model(&models.Session{}).Where("created_at >= ?", today).Count(&todayCount)

	// Calculate total revenue today
	var totalRevenue float64
	h.db.Model(&models.Session{}).
		Where("status = ? AND created_at >= ?", "completed", today).
		Select("COALESCE(SUM(total_fee), 0)").
		Scan(&totalRevenue)

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data": gin.H{
			"active_sessions":    activeCount,
			"completed_sessions": completedCount,
			"today_sessions":     todayCount,
			"today_revenue":      totalRevenue,
		},
	})
}
