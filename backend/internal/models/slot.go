package models

import (
	"time"

	"github.com/google/uuid"
)

// Slot represents a parking slot
type Slot struct {
	ID         uuid.UUID `gorm:"type:uuid;primaryKey;default:gen_random_uuid()" json:"id"`
	Code       string    `gorm:"type:varchar(10);uniqueIndex;not null" json:"code"`
	Zone       string    `gorm:"type:varchar(50);default:'A'" json:"zone"`
	IsOccupied bool      `gorm:"default:false" json:"is_occupied"`
	PositionX  int       `gorm:"default:0" json:"position_x"`
	PositionY  int       `gorm:"default:0" json:"position_y"`
	CreatedAt  time.Time `gorm:"autoCreateTime" json:"created_at"`
	UpdatedAt  time.Time `gorm:"autoUpdateTime" json:"updated_at"`
}

// TableName overrides the table name
func (Slot) TableName() string {
	return "slots"
}

// SlotStats represents parking statistics
type SlotStats struct {
	Total         int     `json:"total"`
	Occupied      int     `json:"occupied"`
	Available     int     `json:"available"`
	OccupancyRate float64 `json:"occupancy_rate"`
}
