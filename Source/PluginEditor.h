#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <memory>

class MessinaAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::AudioProcessorValueTreeState::Listener {
public:
  MessinaAudioProcessorEditor(MessinaAudioProcessor &);
  ~MessinaAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void bindParameter(const juce::String &paramID);
  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;

private:
  MessinaAudioProcessor &audioProcessor;
  std::unique_ptr<juce::WebBrowserComponent> webComponent;

  std::vector<juce::AudioProcessorValueTreeState::Listener *> paramListeners;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessinaAudioProcessorEditor)
};
