package config

import "os"

// Config holds all configuration for the application
type Config struct {
	// Server
	Port    string
	GinMode string

	// Database
	DatabaseURL string

	// MinIO / S3
	MinioEndpoint  string
	MinioAccessKey string
	MinioSecretKey string
	MinioBucket    string
	MinioUseSSL    bool
}

// Load returns a new Config from environment variables
func Load() *Config {
	return &Config{
		// Server
		Port:    getEnv("PORT", "8080"),
		GinMode: getEnv("GIN_MODE", "debug"),

		// Database
		DatabaseURL: getEnv("DATABASE_URL", "postgres://postgres:postgres123@localhost:5432/smartparking?sslmode=disable"),

		// MinIO / S3
		MinioEndpoint:  getEnv("MINIO_ENDPOINT", "localhost:9000"),
		MinioAccessKey: getEnv("MINIO_ACCESS_KEY", "minioadmin"),
		MinioSecretKey: getEnv("MINIO_SECRET_KEY", "minioadmin123"),
		MinioBucket:    getEnv("MINIO_BUCKET", "parking-images"),
		MinioUseSSL:    getEnv("MINIO_USE_SSL", "false") == "true",
	}
}

// getEnv returns the value of an environment variable or a default value
func getEnv(key, defaultValue string) string {
	if value, exists := os.LookupEnv(key); exists {
		return value
	}
	return defaultValue
}
