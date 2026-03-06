import React from 'react';
import { useJuce } from '../hooks/useJuce';
import './Knob.css';

const Knob = ({ label, value, onChange, min, max, suffix = '', displayFormat }) => {
  const percent = (value - min) / (max - min);
  const angle = -135 + percent * 270;

  const handlePointerDown = (e) => {
    e.target.setPointerCapture(e.pointerId);
    let startY = e.clientY;
    let currentVal = value;

    const move = (ev) => {
      const delta = startY - ev.clientY;
      const step = (max - min) * 0.005;
      let nextVal = Math.min(max, Math.max(min, currentVal + delta * step));
      onChange(nextVal);
      currentVal = nextVal;
      startY = ev.clientY;
    };

    const up = (ev) => {
      e.target.releasePointerCapture(e.pointerId);
      window.removeEventListener('pointermove', move);
      window.removeEventListener('pointerup', up);
    };

    window.addEventListener('pointermove', move);
    window.addEventListener('pointerup', up);
  };

  return (
    <div className="knob-container">
      {label && <div className="knob-label-top">{label}</div>}
      <div
        className="knob-control"
        onPointerDown={handlePointerDown}
      >
        <div className="knob-dial">
          <div
            className="knob-indicator-container"
            style={{ transform: `rotate(${angle}deg)` }}
          >
            <div className="knob-dot" />
          </div>
        </div>
      </div>
      {displayFormat ? (
        <div className="knob-value-bottom">{displayFormat(value)}</div>
      ) : suffix ? (
        <div className="knob-value-bottom">{Math.round(value)}{suffix}</div>
      ) : (
        <div className="knob-value-bottom">{Math.round(percent * 100)}%</div>
      )}
    </div>
  );
};

export default Knob;
