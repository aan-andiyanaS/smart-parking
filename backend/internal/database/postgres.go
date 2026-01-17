package database

import (
	"log"

	"smart-parking/internal/models"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"
)

// Connect establishes a connection to the PostgreSQL database
func Connect(databaseURL string) (*gorm.DB, error) {
	db, err := gorm.Open(postgres.Open(databaseURL), &gorm.Config{
		Logger: logger.Default.LogMode(logger.Warn),
	})
	if err != nil {
		return nil, err
	}

	// Auto migrate models
	err = db.AutoMigrate(&models.Slot{}, &models.Capture{}, &models.Session{})
	if err != nil {
		log.Printf("Warning: AutoMigrate failed: %v", err)
	}

	return db, nil
}
