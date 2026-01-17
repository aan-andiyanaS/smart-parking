package models

import (
	"time"

	"github.com/google/uuid"
)

// Session represents a parking session
type Session struct {
	ID              uuid.UUID  `gorm:"type:uuid;primaryKey;default:gen_random_uuid()" json:"id"`
	SlotID          *uuid.UUID `gorm:"type:uuid" json:"slot_id,omitempty"`
	PlateNumber     string     `gorm:"type:varchar(20)" json:"plate_number,omitempty"`
	EntryTime       time.Time  `gorm:"autoCreateTime" json:"entry_time"`
	ExitTime        *time.Time `json:"exit_time,omitempty"`
	DurationMinutes *int       `json:"duration_minutes,omitempty"`
	TotalFee        *float64   `gorm:"type:decimal(10,2)" json:"total_fee,omitempty"`
	Status          string     `gorm:"type:varchar(20);default:'active'" json:"status"`
	CreatedAt       time.Time  `gorm:"autoCreateTime" json:"created_at"`
}

// TableName overrides the table name
func (Session) TableName() string {
	return "sessions"
}
