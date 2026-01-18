import { useState, useEffect, useRef, useCallback } from 'react'
import { Upload, Camera, Clock, Image, Video, VideoOff, RefreshCw } from 'lucide-react'
import './CameraView.css'

function CameraView({ capture, onUpload, streamUrl, aiServiceUrl }) {
    const [isDragging, setIsDragging] = useState(false)
    const [isUploading, setIsUploading] = useState(false)
    const [isStreaming, setIsStreaming] = useState(true)
    const [streamError, setStreamError] = useState(false)
    const [overlayData, setOverlayData] = useState(null)

    const fileInputRef = useRef(null)
    const canvasRef = useRef(null)
    const streamRef = useRef(null)

    // Stream URL - use prop or default to API route
    const actualStreamUrl = streamUrl || '/api/stream'
    const actualAiUrl = aiServiceUrl || (import.meta.env.VITE_AI_SERVICE_URL || 'http://localhost:5000')

    // Fetch overlay data from AI service
    const fetchOverlay = useCallback(async () => {
        try {
            const response = await fetch(`${actualAiUrl}/overlay`)
            if (response.ok) {
                const data = await response.json()
                if (data.success) {
                    setOverlayData(data)
                }
            }
        } catch (err) {
            console.log('Overlay fetch failed:', err.message)
        }
    }, [actualAiUrl])

    // Fetch overlay periodically
    useEffect(() => {
        if (isStreaming) {
            fetchOverlay()
            const interval = setInterval(fetchOverlay, 5000)
            return () => clearInterval(interval)
        }
    }, [isStreaming, fetchOverlay])

    // Draw overlay on canvas
    useEffect(() => {
        if (!overlayData || !overlayData.regions || !canvasRef.current || !streamRef.current) {
            return
        }

        const canvas = canvasRef.current
        const video = streamRef.current
        const ctx = canvas.getContext('2d')

        // Match canvas size to video
        const rect = video.getBoundingClientRect()
        canvas.width = rect.width
        canvas.height = rect.height

        // Clear canvas
        ctx.clearRect(0, 0, canvas.width, canvas.height)

        // Calculate scale factors (assuming VGA 640x480 from ESP32)
        const scaleX = canvas.width / 640
        const scaleY = canvas.height / 480

        // Draw each parking region
        overlayData.regions.forEach(region => {
            const isOccupied = region.is_occupied

            // Set colors
            ctx.strokeStyle = isOccupied ? '#ef4444' : '#22c55e' // red or green
            ctx.fillStyle = isOccupied ? 'rgba(239, 68, 68, 0.2)' : 'rgba(34, 197, 94, 0.2)'
            ctx.lineWidth = 2

            // Draw polygon
            if (region.points && region.points.length > 0) {
                ctx.beginPath()
                const startX = region.points[0][0] * scaleX
                const startY = region.points[0][1] * scaleY
                ctx.moveTo(startX, startY)

                for (let i = 1; i < region.points.length; i++) {
                    const x = region.points[i][0] * scaleX
                    const y = region.points[i][1] * scaleY
                    ctx.lineTo(x, y)
                }
                ctx.closePath()
                ctx.fill()
                ctx.stroke()

                // Draw label
                const centerX = region.points.reduce((sum, p) => sum + p[0], 0) / region.points.length * scaleX
                const centerY = region.points.reduce((sum, p) => sum + p[1], 0) / region.points.length * scaleY

                ctx.font = 'bold 14px Arial'
                ctx.textAlign = 'center'
                ctx.textBaseline = 'middle'

                // Background for label
                const text = `${region.code}: ${isOccupied ? 'TERISI' : 'KOSONG'}`
                const textWidth = ctx.measureText(text).width
                ctx.fillStyle = 'rgba(0, 0, 0, 0.7)'
                ctx.fillRect(centerX - textWidth / 2 - 4, centerY - 10, textWidth + 8, 20)

                // Label text
                ctx.fillStyle = isOccupied ? '#ef4444' : '#22c55e'
                ctx.fillText(text, centerX, centerY)
            }
        })
    }, [overlayData])

    // Handle stream load
    const handleStreamLoad = () => {
        setStreamError(false)
    }

    // Handle stream error
    const handleStreamError = () => {
        setStreamError(true)
    }

    // Retry stream
    const retryStream = () => {
        setStreamError(false)
        if (streamRef.current) {
            // Force reload by changing src
            const currentSrc = streamRef.current.src
            streamRef.current.src = ''
            setTimeout(() => {
                streamRef.current.src = currentSrc
            }, 100)
        }
    }

    // Toggle streaming mode
    const toggleStreaming = () => {
        setIsStreaming(!isStreaming)
    }

    // Drag and drop handlers
    const handleDragOver = (e) => {
        e.preventDefault()
        setIsDragging(true)
    }

    const handleDragLeave = () => {
        setIsDragging(false)
    }

    const handleDrop = async (e) => {
        e.preventDefault()
        setIsDragging(false)

        const files = e.dataTransfer.files
        if (files.length > 0) {
            await handleFile(files[0])
        }
    }

    const handleFileSelect = async (e) => {
        const files = e.target.files
        if (files.length > 0) {
            await handleFile(files[0])
        }
    }

    const handleFile = async (file) => {
        if (!file.type.startsWith('image/')) {
            alert('Please select an image file')
            return
        }

        setIsUploading(true)
        try {
            if (onUpload) {
                await onUpload(file)
            }
        } catch (err) {
            console.error('Upload error:', err)
        } finally {
            setIsUploading(false)
        }
    }

    const formatTime = (timestamp) => {
        if (!timestamp) return 'Never'
        return new Date(timestamp).toLocaleString('id-ID', {
            dateStyle: 'short',
            timeStyle: 'medium'
        })
    }

    return (
        <div className="camera-view">
            {/* Stream/Image Display */}
            <div
                className={`camera-display ${isDragging ? 'dragging' : ''}`}
                onDragOver={handleDragOver}
                onDragLeave={handleDragLeave}
                onDrop={handleDrop}
            >
                {isStreaming ? (
                    <div className="stream-container">
                        {/* MJPEG Stream */}
                        <img
                            ref={streamRef}
                            src={actualStreamUrl}
                            alt="Live stream"
                            className="live-stream"
                            onLoad={handleStreamLoad}
                            onError={handleStreamError}
                        />

                        {/* Detection Overlay Canvas */}
                        <canvas
                            ref={canvasRef}
                            className="detection-overlay"
                        />

                        {/* Stream Error */}
                        {streamError && (
                            <div className="stream-error">
                                <VideoOff size={48} />
                                <p>Stream tidak tersedia</p>
                                <button className="btn btn-secondary" onClick={retryStream}>
                                    <RefreshCw size={16} />
                                    Coba Lagi
                                </button>
                            </div>
                        )}

                        {/* Live indicator */}
                        {!streamError && (
                            <div className="live-indicator">
                                <span className="live-dot"></span>
                                LIVE
                            </div>
                        )}

                        {/* Detection info */}
                        {overlayData && overlayData.last_detection && (
                            <div className="detection-info">
                                <span>🚗 {overlayData.last_detection.vehicles_detected} terdeteksi</span>
                                <span>⏱️ {formatTime(overlayData.last_detection.timestamp)}</span>
                            </div>
                        )}
                    </div>
                ) : capture && capture.image_url ? (
                    <img
                        src={capture.image_url}
                        alt="Latest capture"
                        className="capture-image"
                    />
                ) : (
                    <div className="no-capture">
                        <Image size={48} />
                        <p>No capture available</p>
                        <p className="hint">Drag & drop an image or click upload</p>
                    </div>
                )}

                {/* Drag overlay */}
                {isDragging && (
                    <div className="drag-overlay">
                        <Upload size={48} />
                        <p>Drop image here</p>
                    </div>
                )}

                {/* Upload indicator */}
                {isUploading && (
                    <div className="upload-overlay">
                        <div className="spinner"></div>
                        <p>Uploading...</p>
                    </div>
                )}
            </div>

            {/* Info & Actions */}
            <div className="camera-info">
                <div className="camera-meta">
                    <span className="meta-item">
                        <Camera size={14} />
                        {capture?.camera_id || 'ESP32-CAM'}
                    </span>
                    <span className="meta-item">
                        <Clock size={14} />
                        {formatTime(capture?.captured_at)}
                    </span>
                </div>

                <div className="camera-actions">
                    <button
                        className={`btn ${isStreaming ? 'btn-success' : 'btn-secondary'}`}
                        onClick={toggleStreaming}
                    >
                        {isStreaming ? <Video size={16} /> : <VideoOff size={16} />}
                        {isStreaming ? 'Live' : 'Snapshot'}
                    </button>
                    <input
                        type="file"
                        ref={fileInputRef}
                        accept="image/*"
                        onChange={handleFileSelect}
                        style={{ display: 'none' }}
                    />
                    <button
                        className="btn btn-primary"
                        onClick={() => fileInputRef.current?.click()}
                        disabled={isUploading}
                    >
                        <Upload size={16} />
                        Upload
                    </button>
                </div>
            </div>
        </div>
    )
}

export default CameraView
