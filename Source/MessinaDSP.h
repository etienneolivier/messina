#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

// MSVC doesn't define M_PI by default
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//==============================================================================
class YinPitchTracker {
public:
  YinPitchTracker() {}

  void prepare(double sr, int maxBlockSize) {
    fs = sr;
    bufferIndex = 0;
    processBuffer.assign(1024, 0.0f);
    difference.assign(512, 0.0f);
    cumulative.assign(512, 0.0f);
  }

  float process(float sample) {
    processBuffer[bufferIndex] = sample;
    bufferIndex = (bufferIndex + 1) & 1023; // Fast modulo vs slow % operator

    // Compute pitch only once per buffer (reduces CPU by 90%)
    if (bufferIndex == 0) {
      currentPitch = computeYin();
    }
    return currentPitch;
  }

private:
  float computeYin() {
    int halfSize = 512;
    std::fill(difference.begin(), difference.end(), 0.0f);

    // 0. Energy Gate (Silence/Breath noise rejection)
    float rmsSq = 0.0f;
    for (int i = 0; i < halfSize; i++) {
      float s = processBuffer[(bufferIndex + i) & 1023];
      rmsSq += s * s;
    }
    // If RMS is extremely low, return unvoiced instantly
    if (rmsSq < 0.00001f * halfSize) {
      for (int i = 0; i < 3; i++)
        lastPitches[i] = 0.0f;
      return 0.0f;
    }

    // 1. Difference function
    for (int tau = 0; tau < halfSize; tau++) {
      float sum = 0.0f;
      for (int i = 0; i < halfSize; i++) {
        int idx1 = (bufferIndex + i) & 1023;
        int idx2 = (bufferIndex + i + tau) & 1023;
        float delta = processBuffer[idx1] - processBuffer[idx2];
        sum += delta * delta;
      }
      difference[tau] = sum;
    }

    // 2. Cumulative mean normalized difference
    cumulative[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfSize; tau++) {
      runningSum += difference[tau];
      cumulative[tau] = difference[tau] * tau / (runningSum + 1e-6f);
    }

    // 3. Absolute threshold (TIGHTER TO PREVENT HARMONIC FALSE MATCHES)
    int tauEstimate = -1;
    float threshold =
        0.10f; // Drop from 0.15f to 0.10f for stricter harmonic rejection
    for (int tau = 2; tau < halfSize; tau++) {
      if (cumulative[tau] < threshold) {
        while (tau + 1 < halfSize && cumulative[tau + 1] < cumulative[tau]) {
          tau++;
        }
        tauEstimate = tau;
        break;
      }
    }

    if (tauEstimate == -1) {
      float minVal = 1.0f;
      for (int tau = 2; tau < halfSize; tau++) {
        if (cumulative[tau] < minVal) {
          minVal = cumulative[tau];
          tauEstimate = tau;
        }
      }

      // [VOICED/UNVOICED GATE]
      // If the absolute best correlation we found is still terrible (>0.25),
      // this is broadband noise/breath/consonant, not a pitch.
      if (minVal > 0.25f) {
        // Reset the history to prevent dragging glides from previous valid
        // notes
        for (int i = 0; i < 3; i++)
          lastPitches[i] = 0.0f;
        return 0.0f;
      }
    }

    if (tauEstimate > 0) {
      float exactTau = (float)tauEstimate;

      // Parabolic interpolation for sub-sample pitch accuracy (Fixes detuning)
      if (tauEstimate > 0 && tauEstimate < halfSize - 1) {
        float s0 = cumulative[tauEstimate - 1];
        float s1 = cumulative[tauEstimate];
        float s2 = cumulative[tauEstimate + 1];
        float denom = 2.0f * (s0 - 2.0f * s1 + s2);
        if (denom != 0.0f) {
          exactTau += (s0 - s2) / denom;
        }
      }

      float pitch = fs / exactTau;
      // Stricter tracking bounds specifically for human voice and lead lines
      if (pitch > 80.0f && pitch < 1000.0f) {

        // Multi-frame consensus weighting to kill transient sub-octave burps
        lastPitches[historyPointer] = pitch;
        historyPointer = (historyPointer + 1) % 3;

        int validCount = 0;
        float pitchSum = 0.0f;
        for (int i = 0; i < 3; i++) {
          if (lastPitches[i] > 80.0f) {
            validCount++;
            pitchSum += lastPitches[i];
          }
        }

        if (validCount >= 2) {
          return pitchSum / (float)validCount;
        }

        return pitch;
      }
    }
    return 0.0f; // Unvoiced
  }

  double fs = 44100.0;
  int bufferIndex = 0;
  std::vector<float> processBuffer;
  std::vector<float> difference;
  std::vector<float> cumulative;
  float currentPitch = 0.0f;

  // Consensus tracking
  float lastPitches[3] = {0.0f};
  int historyPointer = 0;
};

//==============================================================================
class GranularPitchShifter {
public:
  GranularPitchShifter() {}

  void prepare(double sr) {
    fs = std::max(44100.0, sr);
    GRAIN_SIZE = std::max(
        100.0f,
        (float)fs * 0.03f); // Tighten to 30ms grain for faster transients

    delayBuffer.setSize(1, (int)fs * 2); // 2 second max buffer
    delayBuffer.clear();
    writePtr = 0;

    phase1 = 0.0f;
    phase2 = 0.5f; // 180 degrees out of phase

    ratio = 1.0f;
    targetRatio = 1.0f;
  }

  void setRatio(float newRatio, float smoothing = 0.01f) {
    targetRatio = newRatio;
    smoothingCoef = smoothing;
  }
  float process(float input) {
    // Variable smoothing based on UI controls
    ratio = ratio * (1.0f - smoothingCoef) + targetRatio * smoothingCoef;

    int maxSamples = delayBuffer.getNumSamples();
    if (maxSamples <= 0 || GRAIN_SIZE <= 0.0f)
      return 0.0f;

    delayBuffer.setSample(0, writePtr, input);

    // Sync phase advance rate -> completely locks to precise granular jumps
    float phaseInc = std::abs(1.0f - ratio) / GRAIN_SIZE;

    // Exact 1.0 ratio behaves as transparent pass-through (Zero Comb Filtering)
    if (ratio == 1.0f) {
      float out = input;
      writePtr = (writePtr + 1) % maxSamples;
      return out;
    }

    phase1 += phaseInc;
    phase2 += phaseInc;

    // Reset grains independently when they complete their cycle bounds
    if (phase1 >= 1.0f) {
      phase1 -= 1.0f;
    }
    if (phase2 >= 1.0f) {
      phase2 -= 1.0f;
    }

    // Delay time maps phase (0.0=min distance, 1.0=max distance)
    float delay1, delay2;
    if (ratio >= 1.0f) {
      // Pitch UP -> Read pointer scans faster than write pointer (starts fast,
      // catches up)
      delay1 = (1.0f - phase1) * GRAIN_SIZE;
      delay2 = (1.0f - phase2) * GRAIN_SIZE;
    } else {
      // Pitch DOWN -> Read pointer scans slower than write pointer (starts from
      // write pointer, falls behind)
      delay1 = phase1 * GRAIN_SIZE;
      delay2 = phase2 * GRAIN_SIZE;
    }

    // Ensure we don't accidentally read negative indices
    float readPos1 = (float)writePtr - delay1;
    float readPos2 = (float)writePtr - delay2;

    if (readPos1 < 0.0f)
      readPos1 += maxSamples;
    if (readPos2 < 0.0f)
      readPos2 += maxSamples;

    // Bounds clamping
    while (readPos1 >= maxSamples)
      readPos1 -= maxSamples;
    while (readPos2 >= maxSamples)
      readPos2 -= maxSamples;

    // Hanning Window Calculation -> 0 at phase 0, 1 at phase 0.5, 0 at phase 1
    float env1 = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase1));
    float env2 = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase2));

    float out1 = getInterpolatedSample(readPos1) * env1;
    float out2 = getInterpolatedSample(readPos2) * env2;

    float output = out1 + out2;

    writePtr = (writePtr + 1) % maxSamples;

    if (std::isnan(output) || std::isinf(output))
      return 0.0f;

    return output;
  }

private:
  float getInterpolatedSample(float pos) {
    int maxSamples = delayBuffer.getNumSamples();
    if (maxSamples <= 0)
      return 0.0f;

    // Fast float to int casting (truncate)
    int idx1 = (int)pos;
    float frac = pos - (float)idx1;

    // Bounds checking
    if (idx1 >= maxSamples)
      idx1 %= maxSamples;
    if (idx1 < 0) {
      idx1 %= maxSamples;
      if (idx1 < 0)
        idx1 += maxSamples;
    }

    int idx2 = idx1 + 1;
    if (idx2 >= maxSamples)
      idx2 -= maxSamples;

    // Hermite Interpolation (much higher quality than linear for pitch
    // shifting)
    int idx0 = idx1 - 1;
    if (idx0 < 0)
      idx0 += maxSamples;
    int idx3 = idx2 + 1;
    if (idx3 >= maxSamples)
      idx3 -= maxSamples;

    const float *rptr = delayBuffer.getReadPointer(0);
    float y0 = rptr[idx0];
    float y1 = rptr[idx1];
    float y2 = rptr[idx2];
    float y3 = rptr[idx3];

    // 4-point, 3rd-order Hermite formula
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
  }

  double fs = 44100.0;
  juce::AudioBuffer<float> delayBuffer;
  int writePtr = 0;
  float phase1 = 0.0f;
  float phase2 = 0.5f;
  float GRAIN_SIZE = 1323.0f;

public:
  float getGrainSize() const { return GRAIN_SIZE; }

private:
  float ratio = 1.0f;
  float targetRatio = 1.0f;
  float smoothingCoef = 0.001f;
};

class SimpleHPF {
public:
  void setCutoff(float freq, float sr) {
    float wc = 2.0f * M_PI * freq / sr;
    alpha = wc / (1.0f + wc);
  }
  float process(float input) {
    y1 = y1 + alpha * (input - y1);
    return input - y1;
  }
  float alpha = 0.0f;
  float y1 = 0.0f;
};

class MessinaEngine {
public:
  MessinaEngine() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    pitchTracker.prepare(sampleRate, spec.maximumBlockSize);

    tunerShifter[0].prepare(sampleRate);
    tunerShifter[1].prepare(sampleRate);
    tonicShifter[0].prepare(sampleRate);
    tonicShifter[1].prepare(sampleRate);

    for (int i = 0; i < 4; i++) {
      harmonyShifters[0][i].prepare(sampleRate);
      harmonyShifters[1][i].prepare(sampleRate);
    }

    driveWaveshaper.functionToUse = [](float x) { return std::tanh(x); };

    juce::dsp::ProcessSpec mwSpec = spec;
    mwSpec.numChannels = 1;
    driveWaveshaper.prepare(mwSpec);

    voiceLfoPhases = {0.0f, 0.25f, 0.5f, 0.75f};
    dryDelayBuffer.setSize(2, (int)sampleRate);
    dryDelayBuffer.clear();
    dryWritePtr[0] = 0;
    dryWritePtr[1] = 0;

    hpfGrit[0].setCutoff(4000.0f, sampleRate);
    hpfGrit[1].setCutoff(4000.0f, sampleRate);
    hpfGlimmer[0].setCutoff(7000.0f, sampleRate);
    hpfGlimmer[1].setCutoff(7000.0f, sampleRate);
  }

  void updateParameters(float driveIn, int tonicIn, float speedIn,
                        float randomizeIn, float dryIn, float tunedIn,
                        float tonicMixIn, float harmIn, int pKey, int pScale,
                        bool fEn, float fGlimmer, float fGrit, float mDryWet) {
    drive = driveIn;
    tonicNote = tonicIn;
    tuneSpeed = speedIn;
    randomize = randomizeIn;
    dryLevel = dryIn;
    tunedLevel = tunedIn;
    tonicLevel = tonicMixIn;
    harmonyLevel = harmIn;

    pitchKey = pKey;
    pitchScale = pScale;
    fxEnable = fEn;
    fxGlimmer = fGlimmer;
    fxHfGrit = fGrit;
    masterDryWet = mDryWet;
  }

  float noteToFreq(int note) {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
  }

  int freqToNearestNote(float freq, int key, int scale) {
    if (freq <= 0)
      return 60;
    int note = std::round(69.0f + 12.0f * std::log2(freq / 440.0f));

    if (scale == 0)
      return note; // Chromatic

    int pc = (note - key) % 12;
    if (pc < 0)
      pc += 12;

    int snappedPc = pc;
    if (scale == 1) { // Major
      int majorNotes[] = {0, 2, 4, 5, 7, 9, 11};
      int minDist = 12;
      for (int n : majorNotes) {
        if (std::abs(pc - n) < minDist) {
          minDist = std::abs(pc - n);
          snappedPc = n;
        }
      }
    } else if (scale == 2) { // Minor
      int minorNotes[] = {0, 2, 3, 5, 7, 8, 10};
      int minDist = 12;
      for (int n : minorNotes) {
        if (std::abs(pc - n) < minDist) {
          minDist = std::abs(pc - n);
          snappedPc = n;
        }
      }
    }
    return note - pc + snappedPc;
  }

  void process(juce::AudioBuffer<float> &buffer,
               juce::MidiBuffer &midiMessages) {
    std::vector<int> activeNotes;

    // Process MIDI
    for (const auto meta : midiMessages) {
      auto msg = meta.getMessage();
      if (msg.isNoteOn()) {
        activeNotesState[msg.getNoteNumber() & 127] = true;
      } else if (msg.isNoteOff()) {
        activeNotesState[msg.getNoteNumber() & 127] = false;
      }
    }

    // Clamp extreme octaves from random MIDI spikes
    for (int note = 0; note < 128; ++note) {
      if (activeNotesState[note]) {
        // Enforce playable harmony vocal range C2 (36) to C6 (84)
        int safeNote = std::max(36, std::min(84, note));
        activeNotes.push_back(safeNote);
      }
    }

    float tonicFreq = noteToFreq(tonicNote);

    int numChannels = std::min(2, buffer.getNumChannels());
    int numSamples = buffer.getNumSamples();

    // Clear any extra channels gracefully
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
      buffer.clear(ch, 0, numSamples);
    }

    auto *dataL = buffer.getWritePointer(0);
    auto *dataR = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i) {
      float dryL = dataL[i];
      float drivenL =
          driveWaveshaper.processSample(dryL * drive) / std::max(0.001f, drive);

      // 1. Calculate Mono Pitch & Voicing Once Per Sample
      float detectedPitch = pitchTracker.process(drivenL);
      if (detectedPitch > 0) {
        isVoiced = true;
        float ratio = detectedPitch / lastValidPitch;
        if (ratio > 0.5f && ratio < 2.0f) {
          // Instantaneous tracking (rely on YIN's internal averaging)
          // Prevents the lag that pulls tuning out of phase
          float pitchCoef = 1.0f - std::exp(-2.0f * M_PI * 100.0f / sampleRate);
          lastValidPitch =
              lastValidPitch * (1.0f - pitchCoef) + detectedPitch * pitchCoef;
        } else {
          lastValidPitch = detectedPitch;
        }
        lastValidPitch = std::max(80.0f, std::min(1000.0f, lastValidPitch));
      } else {
        isVoiced = false;
      }

      // Calculate base harmony intervals off the Tonic core
      std::vector<float> harmonyRatios(4, 1.0f);
      for (int v = 0; v < 4; v++) {
        if (v < activeNotes.size()) {
          float noteFreq = noteToFreq(activeNotes[v]);
          harmonyRatios[v] =
              noteFreq / tonicFreq; // Constant pitch shift from Tonic anchor
        }
      }

      // 2. Base Tuning Ratios
      float tunerRatio = 1.0f;
      float tonicRatio = 1.0f;

      if (isVoiced && lastValidPitch > 0) {
        int nearestNote =
            freqToNearestNote(lastValidPitch, pitchKey, pitchScale);
        float nearestFreq = noteToFreq(nearestNote);
        tunerRatio = nearestFreq / lastValidPitch;
        tonicRatio = tonicFreq / lastValidPitch;

        tunerRatio = std::max(0.5f, std::min(2.0f, tunerRatio));
        tonicRatio = std::max(0.25f, std::min(4.0f, tonicRatio));
      } else if (!isVoiced) {
        tunerRatio = 1.0f;
        tonicRatio = 1.0f;
      }

      float tuneCoef = 1.0f;
      if (tuneSpeed > 0.1f) {
        // Correct time constant to match UI milliseconds
        float tc = std::max(0.001f, tuneSpeed / 1000.0f);
        tuneCoef = 1.0f - std::exp(-1.0f / (tc * sampleRate));
      }

      if (!isVoiced) {
        tuneCoef = 1.0f - std::exp(-1.0f / (0.05f * sampleRate));
      }

      // Advance LFO once per sample (NOT per channel to prevent phase smearing)
      int activeVoices = std::max(1, (int)activeNotes.size());
      if (isVoiced && randomize > 0.01f) {
        for (int v = 0; v < activeVoices && v < 4; v++) {
          voiceLfoPhases[v] += 0.00005f;
          if (voiceLfoPhases[v] > 1.0f)
            voiceLfoPhases[v] -= 1.0f;
        }
      }

      // 3. Process Left & Right Output Channels
      for (int ch = 0; ch < numChannels; ++ch) {
        float drySample = (ch == 0) ? dryL : dataR[i];
        float drivenSample =
            (ch == 0) ? drivenL
                      : (driveWaveshaper.processSample(drySample * drive) /
                         std::max(0.001f, drive));

        tunerShifter[ch].setRatio(tunerRatio, tuneCoef);
        tonicShifter[ch].setRatio(tonicRatio, isVoiced ? 0.05f : tuneCoef);

        float tunedSample = tunerShifter[ch].process(drivenSample);
        float tonicSample = tonicShifter[ch].process(drivenSample);

        float harmonySampleSum = 0.0f;
        for (int v = 0; v < activeVoices && v < 4; v++) {
          float baseRatio = isVoiced ? harmonyRatios[v] : 1.0f;
          float randomMod = 1.0f;

          if (isVoiced && randomize > 0.01f) {
            // Apply a robust 90-degree stereo spread offset to the Right
            // channel
            float chPhase = voiceLfoPhases[v] + (ch == 1 ? 0.25f : 0.0f);
            if (chPhase > 1.0f)
              chPhase -= 1.0f;
            float lfo = std::sin(2.0f * M_PI * chPhase);
            randomMod = 1.0f + (lfo * 0.015f * randomize);
          }

          harmonyShifters[ch][v].setRatio(baseRatio * randomMod,
                                          isVoiced ? 0.01f : 0.05f);

          harmonySampleSum += harmonyShifters[ch][v].process(tonicSample);
        }
        harmonySampleSum /= (float)activeVoices;

        if (std::isnan(harmonySampleSum) || std::isinf(harmonySampleSum))
          harmonySampleSum = 0.0f;

        // Dry latency alignment buffer
        dryDelayBuffer.setSample(ch, dryWritePtr[ch], drySample);
        float delaySamples = (tunerShifter[ch].getGrainSize() / 2.0f);
        float readPos = (float)dryWritePtr[ch] - delaySamples;
        if (readPos < 0.0f)
          readPos += (float)dryDelayBuffer.getNumSamples();

        float delayedDry = drySample;
        int rIdx1 = (int)readPos;
        int rIdx2 = (rIdx1 + 1) % dryDelayBuffer.getNumSamples();
        float frac = readPos - (float)rIdx1;
        const float *rptr = dryDelayBuffer.getReadPointer(ch);
        if (rptr) {
          delayedDry = rptr[rIdx1] * (1.0f - frac) + rptr[rIdx2] * frac;
        }
        dryWritePtr[ch] =
            (dryWritePtr[ch] + 1) % dryDelayBuffer.getNumSamples();

        // Smooth MIDI Mute Gate (only mutes pitch-shifted effects, preserves
        // dry channel)
        float targetGate = activeNotes.empty() ? 0.0f : 1.0f;
        midiGate[ch] = midiGate[ch] * 0.995f +
                       targetGate * 0.005f; // clickless fast attack/release

        float finalOut = (delayedDry * dryLevel) +
                         midiGate[ch] * ((tunedSample * tunedLevel) +
                                         (tonicSample * tonicLevel) +
                                         (harmonySampleSum * harmonyLevel));

        if (fxEnable) {
          float grit =
              std::tanh(hpfGrit[ch].process(finalOut) * 5.0f) * fxHfGrit;
          float glimmer = hpfGlimmer[ch].process(finalOut) * fxGlimmer * 2.5f;
          finalOut += grit + glimmer;
        }

        // Master Dry/Wet crossfade logic
        // drySample is the cleanly delayed dry signal (latency compensated)
        finalOut = delayedDry * (1.0f - masterDryWet) + finalOut * masterDryWet;

        // Peak compression to prevent brutal clipping from high feedback/grit
        finalOut = std::tanh(finalOut);

        if (std::isnan(finalOut) || std::isinf(finalOut))
          finalOut = 0.0f;

        if (ch == 0)
          dataL[i] = finalOut;
        else
          dataR[i] = finalOut;
      }
    }
  }

private:
  double sampleRate = 44100.0;

  YinPitchTracker pitchTracker;
  float lastValidPitch = 440.0f;

  GranularPitchShifter tunerShifter[2];
  GranularPitchShifter tonicShifter[2];
  GranularPitchShifter harmonyShifters[2][4];

  juce::dsp::WaveShaper<float> driveWaveshaper;
  juce::AudioBuffer<float> dryDelayBuffer;
  int dryWritePtr[2] = {0, 0};
  float midiGate[2] = {0.0f, 0.0f};

  bool activeNotesState[128] = {false};
  std::vector<float> voiceLfoPhases;
  bool isVoiced = false;

  float drive = 1.0f;
  int tonicNote = 60;
  float tuneSpeed = 10.0f;
  float randomize = 0.2f;

  float dryLevel = 0.5f;
  float tunedLevel = 0.2f;
  float tonicLevel = 0.5f;
  float harmonyLevel = 0.8f;

  int pitchKey = 0;
  int pitchScale = 0;
  bool fxEnable = true;
  float fxGlimmer = 0.2f;
  float fxHfGrit = 0.2f;
  float masterDryWet = 1.0f;

  SimpleHPF hpfGrit[2];
  SimpleHPF hpfGlimmer[2];
};
