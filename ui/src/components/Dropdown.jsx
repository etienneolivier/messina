import React from 'react';
import './Dropdown.css';

const Dropdown = ({ label, value, options, onChange }) => {
    return (
        <div className="dropdown-container">
            <div className="dropdown-label">{label}</div>
            <div className="dropdown-select-wrapper">
                <select
                    className="dropdown-select"
                    value={value}
                    onChange={(e) => onChange(Number(e.target.value))}
                >
                    {options.map((opt, i) => (
                        <option key={i} value={opt.value !== undefined ? opt.value : i}>
                            {opt.label || opt}
                        </option>
                    ))}
                </select>
                <span className="dropdown-arrow">▼</span>
            </div>
        </div>
    );
};

export default Dropdown;
