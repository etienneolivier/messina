import React from 'react';
import { useJuce } from './hooks/useJuce';
import Knob from './components/Knob';
import MixSlider from './components/MixSlider';
import Dropdown from './components/Dropdown';
import ToggleSwitch from './components/ToggleSwitch';
import logo from '../public/logo.png';
import { Eye, Mic2, Hexagon, Layers, ArrowRightLeft, Sun, Moon } from 'lucide-react';
import './App.css';

const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
const SCALES = ["Chromatic", "Major", "Minor"];

function App() {
  const [inputDrive, setInputDrive] = useJuce('input_drive', 1.0);
  const [tonicNote, setTonicNote] = useJuce('tonic_note', 60.0);
  const [tuneSpeed, setTuneSpeed] = useJuce('tune_speed', 10.0);
  const [randomize, setRandomize] = useJuce('harmony_randomize', 0.2);

  const [mixDry, setMixDry] = useJuce('mix_dry', 0.5);
  const [mixTuned, setMixTuned] = useJuce('mix_tuned', 0.2);
  const [mixTonic, setMixTonic] = useJuce('mix_tonic', 0.5);
  const [mixHarmony, setMixHarmony] = useJuce('mix_harmony', 0.8);

  const [pitchKey, setPitchKey] = useJuce('pitch_key', 0);
  const [pitchScale, setPitchScale] = useJuce('pitch_scale', 0);

  const [fxEnable, setFxEnable] = useJuce('fx_enable', 1);
  const [fxGlimmer, setFxGlimmer] = useJuce('fx_glimmer', 0.2);
  const [fxHfGrit, setFxHfGrit] = useJuce('fx_hf_grit', 0.2);
  const [masterDryWet, setMasterDryWet] = useJuce('master_dry_wet', 1.0);

  const [vibeOn, setVibeOn] = React.useState(false);
  const [lightMode, setLightMode] = React.useState(false);

  // Ask backend to push its current state down so UI syncs upon reopen
  React.useEffect(() => {
    if (window.__JUCE__ && typeof window.__JUCE__.getNativeFunction === 'function') {
      const reqState = window.__JUCE__.getNativeFunction('requestState');
      if (reqState) reqState();
    } else if (typeof window.getNativeFunction === 'function') {
      try {
        window.getNativeFunction('requestState')();
      } catch (e) { }
    }
  }, []);

  const themeClass = lightMode && vibeOn ? 'theme-light' : 'theme-dark';
  const vibeClass = vibeOn ? 'vibe-active' : '';

  return (
    <div className={`app-container ${themeClass} ${vibeClass}`}>
      <div className="aura-bg" />
      <header className="plugin-header">
        <h1>messina <span className="motto">have mercy Chris, it's free software!</span></h1>
        <div className="header-controls">
          {vibeOn && (
            <div className="theme-toggle" onClick={() => setLightMode(!lightMode)}>
              {lightMode ? <Moon size={16} /> : <Sun size={16} />}
            </div>
          )}
          <div className="aura-toggle-wrap">
            <Eye size={14} className="vibe-icon" />
            <span className="aura-label">vibe</span>
            <ToggleSwitch checked={vibeOn} onChange={setVibeOn} />
          </div>
        </div>
      </header>

      <div className="main-grid">

        {/* CONNECTION LINES */}
        <div className="cross-line-1" />
        <div className="cross-line-2" />
        <div className="cross-line-3" />

        {/* --- LEFT COLUMN --- */}
        <div className="column left-col">
          <div className="module pitch-corrector">
            <h2 className="module-title"><Mic2 size={18} /> pitch corrector</h2>
            <div className="pc-body">
              <div className="pc-left">
                <div className="dropdowns-group">
                  <Dropdown
                    label="KEY"
                    value={pitchKey}
                    options={NOTE_NAMES.map((lbl, val) => ({ label: lbl, value: val }))}
                    onChange={setPitchKey}
                  />
                  <Dropdown
                    label="SCALE"
                    value={pitchScale}
                    options={SCALES.map((lbl, val) => ({ label: lbl, value: val }))}
                    onChange={setPitchScale}
                  />
                </div>
                <MixSlider
                  label="VOLUME"
                  value={mixTuned}
                  onChange={setMixTuned}
                  min={0} max={1.0}
                  displayFormat={(v) => `${Math.floor(v * 100)}%`}
                />
              </div>
              <div className="pc-right knob-wrap">
                <Knob
                  label="speed"
                  value={tuneSpeed}
                  onChange={setTuneSpeed}
                  min={0} max={100}
                  suffix="ms"
                />
              </div>
            </div>
          </div>

          <div className="vert-line" />

          <div className="module pitch-equalizer">
            <h2 className="module-title"><ArrowRightLeft size={18} /> pitch neutralizer</h2>
            <div className="pe-row">
              <Dropdown
                label="TONIC NOTE"
                value={Math.floor(tonicNote) % 12}
                options={NOTE_NAMES.map((lbl, val) => ({ label: lbl, value: val }))}
                onChange={(i) => {
                  let octave = Math.floor(tonicNote / 12) - 1;
                  setTonicNote((octave + 1) * 12 + i);
                }}
              />
              <MixSlider
                label="VOLUME"
                value={mixTonic}
                onChange={setMixTonic}
                min={0} max={1.0}
                displayFormat={(v) => `${Math.floor(v * 100)}%`}
              />
            </div>
          </div>
        </div>

        {/* --- RIGHT COLUMN --- */}
        <div className="column right-col">
          <div className="module h3000-harmonizer">
            <div className="h3000-header">
              <h2 className="module-title darker"><Layers size={18} /> H3000 harmonizer</h2>
              <div className="voice-toggle">
                <div className="pip active" />
                <span className="voice-label">4 voices</span>
              </div>
            </div>

            <div className="h3000-body">
              <div className="sliders-col">
                <MixSlider
                  label="INPUT DRIVE"
                  value={inputDrive}
                  onChange={setInputDrive}
                  min={1.0} max={10.0}
                  displayFormat={(v) => `${v.toFixed(1)}`}
                />
                <MixSlider
                  label="VOLUME"
                  value={mixHarmony}
                  onChange={setMixHarmony}
                  min={0} max={1.0}
                  displayFormat={(v) => `${v.toFixed(1)}`}
                />
              </div>
              <div className="knob-wrap">
                <Knob
                  label="chaos LFO"
                  value={randomize}
                  onChange={setRandomize}
                  min={0} max={1.0}
                  displayFormat={(v) => `${Math.floor(v * 100)}%`}
                />
              </div>
            </div>
          </div>

          <div className="vert-line" />

          <div className="module fx-module">
            <div className="fx-header">
              <h2 className="module-title"><Hexagon size={18} /> fx</h2>
              <div className="fx-toggle-wrap">
                <ToggleSwitch checked={fxEnable} onChange={setFxEnable} />
              </div>
            </div>

            <div className="fx-body">
              <MixSlider
                label="GLIMMER"
                value={fxGlimmer}
                onChange={setFxGlimmer}
                min={0} max={1.0}
                displayFormat={(v) => `${Math.floor(v * 100)}%`}
              />
              <MixSlider
                label="HF GRIT"
                value={fxHfGrit}
                onChange={setFxHfGrit}
                min={0} max={1.0}
                displayFormat={(v) => `${Math.floor(v * 100)}%`}
              />
            </div>
          </div>
        </div>
      </div>

      <footer className="plugin-footer">
        <div className="logo-section">
          <img src={logo} className="real-logo" alt="logo" />
          <div className="brand-text">
            <span className="brand-name">malcolm audio</span>
            <span className="brand-hover-text">nobody should pay to create</span>
          </div>
        </div>

        <div className="crossfader-section">
          <span className="cf-label">DRY</span>
          <div className="crossfader">
            <div className="cf-ticks">
              {[...Array(21)].map((_, i) => <div key={i} className={`tick ${i === 10 ? 'center' : ''}`} />)}
            </div>
            <input type="range" className="cf-input" min="0" max="1" step="0.01" value={masterDryWet} onChange={e => setMasterDryWet(Number(e.target.value))} />
          </div>
          <span className="cf-label">WET</span>
        </div>
      </footer>
    </div>
  );
}

export default App;
