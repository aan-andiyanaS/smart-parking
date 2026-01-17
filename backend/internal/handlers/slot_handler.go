package handlers

import (
	"net/http"

	"smart-parking/internal/models"
	"smart-parking/pkg/websocket"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"
)

// SlotHandler handles slot-related HTTP requests
type SlotHandler struct {
	db  *gorm.DB
	hub *websocket.Hub
}

// NewSlotHandler creates a new SlotHandler
func NewSlotHandler(db *gorm.DB, hub *websocket.Hub) *SlotHandler {
	return &SlotHandler{db: db, hub: hub}
}

// GetAll returns all parking slots
func (h *SlotHandler) GetAll(c *gin.Context) {
	var slots []models.Slot

	if err := h.db.Order("position_y, position_x").Find(&slots).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to fetch slots",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    slots,
	})
}

// GetByID returns a single slot by ID
func (h *SlotHandler) GetByID(c *gin.Context) {
	id := c.Param("id")

	var slot models.Slot
	if err := h.db.First(&slot, "id = ?", id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{
			"success": false,
			"error":   "Slot not found",
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    slot,
	})
}

// UpdateRequest represents the update request body
type UpdateRequest struct {
	IsOccupied *bool `json:"is_occupied"`
}

// Update updates a slot's status
func (h *SlotHandler) Update(c *gin.Context) {
	id := c.Param("id")

	var req UpdateRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "Invalid request body",
		})
		return
	}

	var slot models.Slot
	if err := h.db.First(&slot, "id = ?", id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{
			"success": false,
			"error":   "Slot not found",
		})
		return
	}

	// Update field if provided
	if req.IsOccupied != nil {
		slot.IsOccupied = *req.IsOccupied
	}

	if err := h.db.Save(&slot).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to update slot",
		})
		return
	}

	// Broadcast update via WebSocket
	h.hub.Broadcast(websocket.Message{
		Type: "slot_update",
		Data: slot,
	})

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    slot,
	})
}

// Toggle toggles a slot's occupied status
func (h *SlotHandler) Toggle(c *gin.Context) {
	id := c.Param("id")

	// Parse UUID
	slotID, err := uuid.Parse(id)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"success": false,
			"error":   "Invalid slot ID",
		})
		return
	}

	var slot models.Slot
	if err := h.db.First(&slot, "id = ?", slotID).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{
			"success": false,
			"error":   "Slot not found",
		})
		return
	}

	// Toggle status
	slot.IsOccupied = !slot.IsOccupied

	if err := h.db.Save(&slot).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"success": false,
			"error":   "Failed to toggle slot",
		})
		return
	}

	// Broadcast update via WebSocket
	h.hub.Broadcast(websocket.Message{
		Type: "slot_update",
		Data: slot,
	})

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    slot,
	})
}

// GetStats returns parking statistics
func (h *SlotHandler) GetStats(c *gin.Context) {
	var stats models.SlotStats
	var totalCount, occupiedCount int64

	// Count total slots
	h.db.Model(&models.Slot{}).Count(&totalCount)
	stats.Total = int(totalCount)

	// Count occupied slots
	h.db.Model(&models.Slot{}).Where("is_occupied = ?", true).Count(&occupiedCount)
	stats.Occupied = int(occupiedCount)

	// Calculate available
	stats.Available = stats.Total - stats.Occupied

	// Calculate occupancy rate
	if stats.Total > 0 {
		stats.OccupancyRate = float64(stats.Occupied) / float64(stats.Total) * 100
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data":    stats,
	})
}
