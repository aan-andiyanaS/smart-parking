import axios from 'axios'

// Create axios instance
// When VITE_API_URL is not set, use relative paths (for nginx proxy)
const api = axios.create({
    baseURL: import.meta.env.VITE_API_URL || '',
    timeout: 30000,
    headers: {
        'Content-Type': 'application/json',
    },
})

// Response interceptor for error handling
api.interceptors.response.use(
    (response) => response.data,
    (error) => {
        console.error('API Error:', error)
        throw error
    }
)

// ==========================================
// Slots API
// ==========================================

export const getSlots = async () => {
    return api.get('/api/slots')
}

export const getSlotById = async (id) => {
    return api.get(`/api/slots/${id}`)
}

export const updateSlot = async (id, data) => {
    return api.put(`/api/slots/${id}`, data)
}

export const toggleSlot = async (id) => {
    return api.post(`/api/slots/${id}/toggle`)
}

export const getStats = async () => {
    return api.get('/api/slots/stats')
}

// ==========================================
// Capture API
// ==========================================

export const getLatestCapture = async () => {
    return api.get('/api/capture/latest')
}

export const getCaptures = async () => {
    return api.get('/api/captures')
}

export const uploadImage = async (file, cameraId = 'cam1') => {
    const formData = new FormData()
    formData.append('image', file)
    formData.append('camera_id', cameraId)

    return api.post('/api/capture', formData, {
        headers: {
            'Content-Type': 'multipart/form-data',
        },
    })
}

// ==========================================
// Stream API
// ==========================================

export const getStreamStatus = async () => {
    return api.get('/api/stream/status')
}

export const setStreamUrl = async (url) => {
    return api.put('/api/stream/url', { url })
}

export default api
