# Messina

Messina is a dedicated Granular Vocal Harmonizer and Pitch Shifter JUCE plugin designed for modern, experimental vocal processing.

## Overview

Messina utilizes a custom-built Granular Pitch Shifter combined with an highly optimized, real-time Yin Pitch Tracker to provide clean and fast vocal tuning, harmonization, and pitch-shifting effects. It allows for advanced MIDI routing to control up to four distinct harmony voices simultaneously.

## Features

- **Robust Pitch Tracking:** A heavily optimized Yin algorithm designed specifically to track human voice fundamentals quickly while rejecting breath noise, consonants, and false harmonics.
- **Granular Pitch Shifting:** A clean, transient-preserving granular engine with variable smoothing, allowing precise pitch transposition without the "chipmunk" or "Darth Vader" formants typical of standard resamplers.
- **Polyphonic Harmonization:** Route MIDI input to Messina to dynamically trigger up to four harmony voices. It intelligently follows your playing and anchors to the root note or scale.
- **Scale Snap & Auto-Tune:** Built-in scale snapping (Chromatic, Major, Minor) with variable retune speed for T-Pain-style hard tuning or natural vibrato preservation.
- **Stereo Randomized Spread:** Adds a 90-degree phase-offset LFO modulation to duplicate voices, creating massive, wide vocal stacks.
- **Integrated FX:** Includes integrated high-pass Grit (saturation) and Glimmer (exciter) circuits for adding bite and air to the output stage.
- **Modern Web UI:** Features a sleek, responsive React-based interface embedded directly via JUCE's WebView.

## Architecture & Tech Stack

- **C++20 / JUCE:** High-performance DSP backend processing.
- **React / Vite / Web Technologies:** Provides a highly flexible, modern, and aesthetically pleasing parameter control frontend (`ui.html`).

## Installation

### Prerequisites
- CMake 3.22+
- Supported Host DAW (Ableton, Logic, FL Studio, etc.)

### Build Instructions

1. Clone this repository.
2. Initialize the JUCE submodule if you haven't already (requires linking to your local JUCE installation or replacing the `add_subdirectory` command in `CMakeLists.txt`).
3. Build using CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

4. The built VST3 and AU plugins will be automatically copied to your user's system plugin folders (MacOS).

## Usage

Insert Messina on a vocal track.
To use the harmonization engine, route a MIDI track's output to the plugin instance (steps vary by DAW). Use the `Tune Speed`, `Randomize`, and `Harmony Level` controls to blend the synthetic voices to taste.
