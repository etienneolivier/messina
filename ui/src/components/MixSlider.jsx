import React from 'react';
import './MixSlider.css';

const MixSlider = ({ label, value, onChange, min, max, showValue = true, displayFormat }) => {
    const percent = (value - min) / (max - min);

    const handlePointerDown = (e) => {
        const track = e.currentTarget;
        track.setPointerCapture(e.pointerId);

        const update = (ev) => {
            const rect = track.getBoundingClientRect();
            const p = Math.max(0, Math.min(1, (ev.clientX - rect.left) / rect.width));
            onChange(min + p * (max - min));
        };

        update(e);

        const up = (ev) => {
            track.releasePointerCapture(e.pointerId);
            window.removeEventListener('pointermove', update);
            window.removeEventListener('pointerup', up);
        };

        window.addEventListener('pointermove', update);
        window.addEventListener('pointerup', up);
    };

    return (
        <div className="mix-slider-container">
            {label && <div className="mix-label">{label}</div>}
            <div className="mix-track-wrapper">
                <div className="mix-track" onPointerDown={handlePointerDown}>
                    <div className="mix-fill" style={{ width: `${percent * 100}%` }} />
                    <div className="mix-thumb" style={{ left: `${percent * 100}%` }}>
                        <div className="mix-thumb-glow" />
                    </div>
                </div>
                {showValue && (
                    <div className="mix-value">
                        {displayFormat ? displayFormat(value) : `${Math.round(percent * 100)}%`}
                    </div>
                )}
            </div>
        </div>
    );
};

export default MixSlider;
