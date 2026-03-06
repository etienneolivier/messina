import React from 'react';
import './ToggleSwitch.css';

const ToggleSwitch = ({ label, onLabel, checked, onChange, showPip = false }) => {
    return (
        <div className={`toggle-switch-container ${checked ? 'on' : 'off'}`} onClick={() => onChange(!checked)}>
            {label && <div className="toggle-switch-label">{label}</div>}
            {showPip && <div className={`toggle-switch-pip ${checked ? 'active' : ''}`} />}
            {onLabel && <div className="toggle-switch-on-label">{onLabel}</div>}
            <div className="toggle-switch-track">
                <div className="toggle-switch-thumb" />
            </div>
        </div>
    );
};

export default ToggleSwitch;
