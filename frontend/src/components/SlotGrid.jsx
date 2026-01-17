import SlotCard from './SlotCard'
import './SlotGrid.css'

function SlotGrid({ slots, onToggle }) {
    if (!slots || slots.length === 0) {
        return (
            <div className="slot-grid-empty">
                <p>No parking slots configured</p>
            </div>
        )
    }

    // Calculate grid dimensions
    const maxX = Math.max(...slots.map(s => s.position_x)) + 1
    const maxY = Math.max(...slots.map(s => s.position_y)) + 1

    return (
        <div
            className="slot-grid"
            style={{
                gridTemplateColumns: `repeat(${maxX}, 1fr)`,
                gridTemplateRows: `repeat(${maxY}, 1fr)`
            }}
        >
            {slots.map(slot => (
                <SlotCard
                    key={slot.id}
                    slot={slot}
                    onToggle={onToggle}
                />
            ))}
        </div>
    )
}

export default SlotGrid
