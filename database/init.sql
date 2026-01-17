-- ===========================================
-- Smart Parking System - Database Schema
-- ===========================================

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- ===========================================
-- SLOTS TABLE
-- ===========================================
CREATE TABLE IF NOT EXISTS slots (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    code VARCHAR(10) NOT NULL UNIQUE,
    zone VARCHAR(50) DEFAULT 'A',
    is_occupied BOOLEAN DEFAULT false,
    position_x INTEGER DEFAULT 0,
    position_y INTEGER DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- ===========================================
-- CAPTURES TABLE
-- ===========================================
CREATE TABLE IF NOT EXISTS captures (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    image_url TEXT NOT NULL,
    camera_id VARCHAR(50) DEFAULT 'cam1',
    captured_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- ===========================================
-- SESSIONS TABLE (untuk tracking parkir)
-- ===========================================
CREATE TABLE IF NOT EXISTS sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    slot_id UUID REFERENCES slots(id) ON DELETE SET NULL,
    plate_number VARCHAR(20),
    entry_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    exit_time TIMESTAMP WITH TIME ZONE,
    duration_minutes INTEGER,
    total_fee DECIMAL(10,2),
    status VARCHAR(20) DEFAULT 'active',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- ===========================================
-- SEED DATA - 4 Parking Slots (2 Kiri, 2 Kanan)
-- Layout:
--   P1 | P2   (Row 0)
--   P3 | P4   (Row 1)
-- ===========================================
INSERT INTO slots (code, zone, position_x, position_y, is_occupied) VALUES
    ('P1', 'Kiri', 0, 0, false),   -- Kiri atas
    ('P2', 'Kanan', 1, 0, false),  -- Kanan atas
    ('P3', 'Kiri', 0, 1, false),   -- Kiri bawah
    ('P4', 'Kanan', 1, 1, false)   -- Kanan bawah
ON CONFLICT (code) DO NOTHING;

-- ===========================================
-- INDEXES
-- ===========================================
CREATE INDEX IF NOT EXISTS idx_slots_code ON slots(code);
CREATE INDEX IF NOT EXISTS idx_slots_is_occupied ON slots(is_occupied);
CREATE INDEX IF NOT EXISTS idx_captures_captured_at ON captures(captured_at DESC);
CREATE INDEX IF NOT EXISTS idx_sessions_status ON sessions(status);

-- ===========================================
-- UPDATE TRIGGER - Auto update updated_at
-- ===========================================
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE OR REPLACE TRIGGER update_slots_updated_at
    BEFORE UPDATE ON slots
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();
