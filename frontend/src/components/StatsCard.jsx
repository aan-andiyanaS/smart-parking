import './StatsCard.css'

function StatsCard({ title, value, icon, color = 'primary' }) {
    return (
        <div className={`stats-card stats-card-${color}`}>
            <div className="stats-icon">{icon}</div>
            <div className="stats-content">
                <span className="stats-value">{value}</span>
                <span className="stats-title">{title}</span>
            </div>
        </div>
    )
}

export default StatsCard
