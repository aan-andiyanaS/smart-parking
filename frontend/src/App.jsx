import { useState, useEffect, useCallback } from 'react'
import { Car, Camera, RefreshCw, Wifi, WifiOff, Upload } from 'lucide-react'
import './App.css'
import StatsCard from './components/StatsCard'
import SlotGrid from './components/SlotGrid'
import CameraView from './components/CameraView'
import { getSlots, getStats, getLatestCapture, toggleSlot, uploadImage } from './services/api'
import { connectWebSocket, onMessage, closeWebSocket } from './services/websocket'

function App() {
    const [slots, setSlots] = useState([])
    const [stats, setStats] = useState({ total: 0, occupied: 0, available: 0, occupancy_rate: 0 })
    const [latestCapture, setLatestCapture] = useState(null)
    const [isConnected, setIsConnected] = useState(false)
    const [loading, setLoading] = useState(true)
    const [error, setError] = useState(null)

    // Fetch initial data
    const fetchData = useCallback(async () => {
        try {
            setLoading(true)

            // Use Promise.allSettled to handle individual failures gracefully
            const results = await Promise.allSettled([
                getSlots(),
                getStats(),
                getLatestCapture()
            ])

            // Handle slots result
            if (results[0].status === 'fulfilled' && results[0].value?.success) {
                setSlots(results[0].value.data)
            }

            // Handle stats result
            if (results[1].status === 'fulfilled' && results[1].value?.success) {
                setStats(results[1].value.data)
            }

            // Handle capture result (may be 404 if no captures yet)
            if (results[2].status === 'fulfilled' && results[2].value?.success) {
                setLatestCapture(results[2].value.data)
            }

            // Only show error if slots fetch failed (critical)
            if (results[0].status === 'rejected') {
                setError('Failed to fetch data. Is the backend running?')
            } else {
                setError(null)
            }
        } catch (err) {
            setError('Failed to fetch data. Is the backend running?')
            console.error('Fetch error:', err)
        } finally {
            setLoading(false)
        }
    }, [])

    // Initialize WebSocket
    useEffect(() => {
        fetchData()

        const ws = connectWebSocket()

        ws.onopen = () => {
            setIsConnected(true)
            console.log('WebSocket connected')
        }

        ws.onclose = () => {
            setIsConnected(false)
            console.log('WebSocket disconnected')
        }

        onMessage((message) => {
            console.log('WS Message:', message)

            if (message.type === 'slot_update') {
                // Update specific slot
                setSlots(prev => prev.map(slot =>
                    slot.id === message.data.id ? message.data : slot
                ))
                // Refresh stats
                getStats().then(res => {
                    if (res.success) setStats(res.data)
                })
            }

            if (message.type === 'new_capture') {
                setLatestCapture(message.data)
            }
        })

        return () => {
            closeWebSocket()
        }
    }, [fetchData])

    // Handle slot toggle
    const handleToggleSlot = async (slotId) => {
        try {
            const result = await toggleSlot(slotId)
            if (result.success) {
                // Update will come via WebSocket, but update optimistically
                setSlots(prev => prev.map(slot =>
                    slot.id === slotId ? { ...slot, is_occupied: !slot.is_occupied } : slot
                ))
            }
        } catch (err) {
            console.error('Toggle error:', err)
        }
    }

    // Handle image upload
    const handleUpload = async (file) => {
        try {
            const result = await uploadImage(file)
            if (result.success) {
                setLatestCapture(result.data)
            }
        } catch (err) {
            console.error('Upload error:', err)
        }
    }

    return (
        <div className="app">
            {/* Header */}
            <header className="header">
                <div className="header-content">
                    <div className="header-left">
                        <Car className="logo-icon" />
                        <h1>Smart Parking System</h1>
                    </div>
                    <div className="header-right">
                        <span className={`connection-status ${isConnected ? 'connected' : 'disconnected'}`}>
                            {isConnected ? <Wifi size={16} /> : <WifiOff size={16} />}
                            {isConnected ? 'Connected' : 'Disconnected'}
                        </span>
                        <button className="btn btn-primary" onClick={fetchData}>
                            <RefreshCw size={16} />
                            Refresh
                        </button>
                    </div>
                </div>
            </header>

            {/* Main Content */}
            <main className="main-content">
                {error && (
                    <div className="error-banner">
                        {error}
                        <button onClick={fetchData}>Retry</button>
                    </div>
                )}

                {/* Stats Cards */}
                <section className="stats-section">
                    <StatsCard
                        title="Total Slots"
                        value={stats.total}
                        icon="🅿️"
                        color="primary"
                    />
                    <StatsCard
                        title="Occupied"
                        value={stats.occupied}
                        icon="🚗"
                        color="danger"
                    />
                    <StatsCard
                        title="Available"
                        value={stats.available}
                        icon="✅"
                        color="success"
                    />
                    <StatsCard
                        title="Occupancy Rate"
                        value={`${stats.occupancy_rate.toFixed(1)}%`}
                        icon="📊"
                        color="warning"
                    />
                </section>

                {/* Main Grid */}
                <section className="content-grid">
                    {/* Slot Grid */}
                    <div className="card slot-section">
                        <div className="card-header">
                            <h2>Parking Slots</h2>
                            <span className="subtitle">Click to toggle status</span>
                        </div>
                        {loading ? (
                            <div className="loading">Loading slots...</div>
                        ) : (
                            <SlotGrid slots={slots} onToggle={handleToggleSlot} />
                        )}
                    </div>

                    {/* Camera View */}
                    <div className="card camera-section">
                        <div className="card-header">
                            <h2><Camera size={20} /> Live Camera</h2>
                        </div>
                        <CameraView
                            capture={latestCapture}
                            onUpload={handleUpload}
                        />
                    </div>
                </section>

                {/* Legend */}
                <section className="legend-section">
                    <div className="card legend">
                        <h3>Legend</h3>
                        <div className="legend-items">
                            <div className="legend-item">
                                <span className="legend-color available"></span>
                                <span>Available</span>
                            </div>
                            <div className="legend-item">
                                <span className="legend-color occupied"></span>
                                <span>Occupied</span>
                            </div>
                        </div>
                    </div>
                </section>
            </main>

            {/* Footer */}
            <footer className="footer">
                <p>Smart Parking System &copy; 2026 | SDG 11: Sustainable Cities</p>
            </footer>
        </div>
    )
}

export default App
