#include "PluginProcessor.h"
#include "PluginEditor.h"

MessinaAudioProcessor::MessinaAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

MessinaAudioProcessor::~MessinaAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
MessinaAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"input_drive", 1}, "Input Drive", 1.0f, 10.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"tonic_note", 1}, "Tonic Note", 24, 108,
      60)); // MIDI note (60 = C4)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"tune_speed", 1}, "Retune Speed", 0.0f, 100.0f,
      10.0f)); // ms
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"harmony_randomize", 1}, "Randomize", 0.0f, 1.0f,
      0.2f));

  params.push_back(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"pitch_key", 1}, "Key", 0, 11, 0)); // 0=C, 1=C#...
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"pitch_scale", 1}, "Scale", 0, 2,
      0)); // 0=Chrom, 1=Maj, 2=Min

  params.push_back(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"fx_enable", 1}, "FX Enable", true));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"fx_glimmer", 1}, "Glimmer", 0.0f, 1.0f, 0.2f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"fx_hf_grit", 1}, "HF Grit", 0.0f, 1.0f, 0.2f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"master_dry_wet", 1}, "Dry/Wet", 0.0f, 1.0f, 1.0f));

  // Mixer
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"mix_dry", 1}, "Dry", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"mix_tuned", 1}, "Tuned", 0.0f, 1.0f, 0.2f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"mix_tonic", 1}, "Tonic", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"mix_harmony", 1}, "Harmony", 0.0f, 1.0f, 0.8f));

  return {params.begin(), params.end()};
}

const juce::String MessinaAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool MessinaAudioProcessor::acceptsMidi() const { return true; }
bool MessinaAudioProcessor::producesMidi() const { return false; }
bool MessinaAudioProcessor::isMidiEffect() const { return false; }
double MessinaAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MessinaAudioProcessor::getNumPrograms() { return 1; }
int MessinaAudioProcessor::getCurrentProgram() { return 0; }
void MessinaAudioProcessor::setCurrentProgram(int index) {}
const juce::String MessinaAudioProcessor::getProgramName(int index) {
  return {};
}
void MessinaAudioProcessor::changeProgramName(int index,
                                              const juce::String &newName) {}

void MessinaAudioProcessor::prepareToPlay(double sampleRate,
                                          int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = std::max(44100.0, sampleRate);
  spec.maximumBlockSize = std::max(1, samplesPerBlock);
  // PREPARE FOR UP TO 8 CHANNELS TO BE SAFE ON HIGH CHANNEL SIDECHAINS
  spec.numChannels = std::max(2, getTotalNumOutputChannels());
  messinaEngine.prepare(spec);
}

void MessinaAudioProcessor::releaseResources() {}

void MessinaAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                         juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // If mono-to-stereo, copy the left channel to the right channel BEFORE
  // processing
  if (totalNumInputChannels == 1 && totalNumOutputChannels >= 2) {
    buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    // clear any further extra channels
    for (auto i = 2; i < totalNumOutputChannels; ++i) {
      buffer.clear(i, 0, buffer.getNumSamples());
    }
  } else {
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
      buffer.clear(i, 0, buffer.getNumSamples());
    }
  }

  // Update Parameters
  float drive = apvts.getRawParameterValue("input_drive")->load();
  int tonic = (int)apvts.getRawParameterValue("tonic_note")->load();
  float speed = apvts.getRawParameterValue("tune_speed")->load();
  float randomize = apvts.getRawParameterValue("harmony_randomize")->load();
  float dry = apvts.getRawParameterValue("mix_dry")->load();
  float tuned = apvts.getRawParameterValue("mix_tuned")->load();
  float tonicMix = apvts.getRawParameterValue("mix_tonic")->load();
  float harm = apvts.getRawParameterValue("mix_harmony")->load();

  int pitchKey = (int)apvts.getRawParameterValue("pitch_key")->load();
  int pitchScale = (int)apvts.getRawParameterValue("pitch_scale")->load();

  bool fxEnable = apvts.getRawParameterValue("fx_enable")->load() > 0.5f;
  float fxGlimmer = apvts.getRawParameterValue("fx_glimmer")->load();
  float fxHfGrit = apvts.getRawParameterValue("fx_hf_grit")->load();

  float masterDryWet = apvts.getRawParameterValue("master_dry_wet")->load();

  messinaEngine.updateParameters(drive, tonic, speed, randomize, dry, tuned,
                                 tonicMix, harm, pitchKey, pitchScale, fxEnable,
                                 fxGlimmer, fxHfGrit, masterDryWet);

  // Process
  messinaEngine.process(buffer, midiMessages);
}

bool MessinaAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *MessinaAudioProcessor::createEditor() {
  return new MessinaAudioProcessorEditor(*this);
}

void MessinaAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void MessinaAudioProcessor::setStateInformation(const void *data,
                                                int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new MessinaAudioProcessor();
}
