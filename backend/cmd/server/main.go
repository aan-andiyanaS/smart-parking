package main

import (
	"log"

	"smart-parking/internal/config"
	"smart-parking/internal/database"
	"smart-parking/internal/handlers"
	"smart-parking/internal/middleware"
	"smart-parking/internal/services"
	"smart-parking/pkg/websocket"

	"github.com/gin-gonic/gin"
	"github.com/joho/godotenv"
)

func main() {
	// Load .env file (optional, for local dev)
	godotenv.Load()

	// Load configuration
	cfg := config.Load()

	// Connect to database
	db, err := database.Connect(cfg.DatabaseURL)
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}
	log.Println("✅ Connected to PostgreSQL")

	// Initialize MinIO storage
	storage, err := services.NewMinIOStorage(
		cfg.MinioEndpoint,
		cfg.MinioAccessKey,
		cfg.MinioSecretKey,
		cfg.MinioBucket,
		cfg.MinioUseSSL,
	)
	if err != nil {
		log.Printf("⚠️  MinIO connection failed: %v (uploads will fail)", err)
	} else {
		log.Println("✅ Connected to MinIO")
	}

	// Initialize WebSocket hub
	hub := websocket.NewHub()
	go hub.Run()
	log.Println("✅ WebSocket hub started")

	// Setup Gin router
	if cfg.GinMode == "release" {
		gin.SetMode(gin.ReleaseMode)
	}

	router := gin.Default()

	// Middleware
	router.Use(middleware.CORS())
	router.Use(gin.Recovery())

	// Health check
	router.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "ok"})
	})

	// API routes
	api := router.Group("/api")
	{
		// Slots endpoints
		slotHandler := handlers.NewSlotHandler(db, hub)
		api.GET("/slots", slotHandler.GetAll)
		api.GET("/slots/stats", slotHandler.GetStats)
		api.GET("/slots/:id", slotHandler.GetByID)
		api.PUT("/slots/:id", slotHandler.Update)
		api.POST("/slots/:id/toggle", slotHandler.Toggle)

		// Capture endpoints
		captureHandler := handlers.NewCaptureHandler(db, storage, hub)
		api.POST("/capture", captureHandler.Upload)
		api.GET("/capture/latest", captureHandler.GetLatest)
		api.GET("/captures", captureHandler.GetAll)

		// Stream proxy endpoints (ESP32 camera)
		streamHandler := handlers.NewStreamHandler()
		api.GET("/stream", streamHandler.ProxyStream)
		api.GET("/stream/capture", streamHandler.ProxyCapture)
		api.GET("/stream/status", streamHandler.GetCameraStatus)
		api.PUT("/stream/url", streamHandler.SetCameraURL)
	}

	// WebSocket endpoint
	router.GET("/ws", func(c *gin.Context) {
		websocket.ServeWs(hub, c.Writer, c.Request)
	})

	// Start server
	port := cfg.Port
	if port == "" {
		port = "8080"
	}

	log.Printf("🚀 Server starting on port %s", port)
	if err := router.Run(":" + port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
