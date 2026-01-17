import { useState, useRef } from 'react'
import { Upload, Camera, Clock, Image } from 'lucide-react'
import './CameraView.css'

function CameraView({ capture, onUpload }) {
    const [isDragging, setIsDragging] = useState(false)
    const [isUploading, setIsUploading] = useState(false)
    const fileInputRef = useRef(null)

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
            {/* Image Display */}
            <div
                className={`camera-display ${isDragging ? 'dragging' : ''}`}
                onDragOver={handleDragOver}
                onDragLeave={handleDragLeave}
                onDrop={handleDrop}
            >
                {capture && capture.image_url ? (
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
                        {capture?.camera_id || 'cam1'}
                    </span>
                    <span className="meta-item">
                        <Clock size={14} />
                        {formatTime(capture?.captured_at)}
                    </span>
                </div>

                <div className="camera-actions">
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
                        Upload Image
                    </button>
                </div>
            </div>
        </div>
    )
}

export default CameraView
