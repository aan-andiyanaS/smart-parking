import { Car } from 'lucide-react'
import './SlotCard.css'

function SlotCard({ slot, onToggle }) {
    const handleClick = () => {
        if (onToggle) {
            onToggle(slot.id)
        }
    }

    return (
        <div
            className={`slot-card ${slot.is_occupied ? 'occupied' : 'available'}`}
            onClick={handleClick}
            style={{
                gridColumn: slot.position_x + 1,
                gridRow: slot.position_y + 1
            }}
        >
            <div className="slot-icon">
                {slot.is_occupied ? <Car size={32} /> : <span className="check">✓</span>}
            </div>
            <div className="slot-code">{slot.code}</div>
            <div className="slot-status">
                {slot.is_occupied ? 'Occupied' : 'Available'}
            </div>
        </div>
    )
}

export default SlotCard
