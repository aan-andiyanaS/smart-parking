let ws = null
let messageHandler = null
let reconnectTimer = null
const RECONNECT_DELAY = 3000

// Get WebSocket URL
const getWsUrl = () => {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const host = import.meta.env.VITE_WS_URL || `${protocol}//${window.location.host}/ws`

    // If VITE_WS_URL is defined, use it directly
    if (import.meta.env.VITE_WS_URL) {
        return import.meta.env.VITE_WS_URL
    }

    return `${protocol}//${window.location.host}/ws`
}

// Connect to WebSocket
export const connectWebSocket = () => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        return ws
    }

    const url = getWsUrl()
    console.log('Connecting to WebSocket:', url)

    try {
        ws = new WebSocket(url)

        ws.onopen = () => {
            console.log('WebSocket connected')
            // Clear reconnect timer if exists
            if (reconnectTimer) {
                clearTimeout(reconnectTimer)
                reconnectTimer = null
            }
        }

        ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data)
                if (messageHandler) {
                    messageHandler(message)
                }
            } catch (err) {
                console.error('Failed to parse WebSocket message:', err)
            }
        }

        ws.onerror = (error) => {
            console.error('WebSocket error:', error)
        }

        ws.onclose = () => {
            console.log('WebSocket disconnected')
            // Attempt to reconnect
            if (!reconnectTimer) {
                reconnectTimer = setTimeout(() => {
                    console.log('Attempting to reconnect...')
                    connectWebSocket()
                }, RECONNECT_DELAY)
            }
        }

        return ws
    } catch (err) {
        console.error('Failed to create WebSocket:', err)
        return null
    }
}

// Set message handler
export const onMessage = (handler) => {
    messageHandler = handler
}

// Send message (if needed)
export const sendMessage = (message) => {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(message))
    }
}

// Close WebSocket
export const closeWebSocket = () => {
    if (reconnectTimer) {
        clearTimeout(reconnectTimer)
        reconnectTimer = null
    }
    if (ws) {
        ws.close()
        ws = null
    }
}

// Get connection status
export const isConnected = () => {
    return ws && ws.readyState === WebSocket.OPEN
}

export default {
    connectWebSocket,
    onMessage,
    sendMessage,
    closeWebSocket,
    isConnected,
}
