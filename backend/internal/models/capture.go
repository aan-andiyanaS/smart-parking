package models

import (
	"time"

	"github.com/google/uuid"
)

// Capture represents a camera capture
type Capture struct {
	ID         uuid.UUID `gorm:"type:uuid;primaryKey;default:gen_random_uuid()" json:"id"`
	ImageURL   string    `gorm:"type:text;not null" json:"image_url"`
	CameraID   string    `gorm:"type:varchar(50);default:'cam1'" json:"camera_id"`
	CapturedAt time.Time `gorm:"autoCreateTime" json:"captured_at"`
}

// TableName overrides the table name
func (Capture) TableName() string {
	return "captures"
}
