#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "BinaryData.h"

//==============================================================================
MessinaAudioProcessorEditor::MessinaAudioProcessorEditor(
    MessinaAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

  // Load UI from BinaryData
  auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("Messina_UI");
  tempDir.createDirectory();
  auto uiFile = tempDir.getChildFile("ui.html");
  uiFile.replaceWithData(BinaryData::ui_html, BinaryData::ui_htmlSize);

  juce::String jsPolyfill = R"JS(
    class PromiseHandler {
      constructor() {
        this.lastPromiseId = 0;
        this.promises = new Map();
        window.__JUCE__.backend.addEventListener("__juce__complete", (event) => {
          if (this.promises.has(event.promiseId)) {
            this.promises.get(event.promiseId).resolve(event.result);
            this.promises.delete(event.promiseId);
          }
        });
      }
      createPromise() {
        const promiseId = this.lastPromiseId++;
        return [promiseId, new Promise((resolve, reject) => {
          this.promises.set(promiseId, { resolve, reject });
        })];
      }
    }

    const promiseHandler = new PromiseHandler();

    function getNativeFunction(name) {
      if (window.__JUCE__.initialisationData.__juce__functions && 
          !window.__JUCE__.initialisationData.__juce__functions.includes(name)) {
        console.warn("Creating unknown native function: " + name);
      }
      return function() {
        const [promiseId, result] = promiseHandler.createPromise();
        window.__JUCE__.backend.emitEvent("__juce__invoke", {
          name: name,
          params: Array.prototype.slice.call(arguments),
          resultId: promiseId,
        });
        return result;
      };
    }
    
    // EXPOSE GLOBALLY
    window.getNativeFunction = getNativeFunction;
    window.setParameter = getNativeFunction("setParameter");
    console.log("JUCE Bridge Polyfill Injected. window.setParameter ready.");
  )JS";

  auto options =
      juce::WebBrowserComponent::Options{}
          .withKeepPageLoadedWhenBrowserIsHidden()
          .withAppleWkWebViewOptions(
              juce::WebBrowserComponent::Options::AppleWkWebView{}
                  .withAllowAccessToEnclosingDirectory(true))
          .withUserScript(jsPolyfill)
          .withNativeFunction(
              juce::Identifier("setParameter"),
              [this](const juce::Array<juce::var> &args,
                     juce::WebBrowserComponent::NativeFunctionCompletion
                         completion) {
                if (args.size() == 2 && args[0].isString()) {
                  auto paramId = args[0].toString();
                  auto value = (float)args[1];
                  if (auto *param =
                          audioProcessor.apvts.getParameter(paramId)) {
                    param->setValueNotifyingHost(param->convertTo0to1(value));
                  }
                }
                completion(juce::var());
              })
          .withNativeFunction(
              juce::Identifier("requestState"),
              [this](const juce::Array<juce::var> &args,
                     juce::WebBrowserComponent::NativeFunctionCompletion
                         completion) {
                juce::StringArray paramIds = {
                    "input_drive",       "tonic_note",    "tune_speed",
                    "harmony_randomize", "mix_dry",       "mix_tuned",
                    "mix_tonic",         "mix_harmony",   "pitch_key",
                    "pitch_scale",       "fx_enable",     "fx_glimmer",
                    "fx_hf_grit",        "master_dry_wet"};
                for (const auto &id : paramIds) {
                  if (auto *param = audioProcessor.apvts.getParameter(id)) {
                    parameterChanged(id,
                                     param->convertFrom0to1(param->getValue()));
                  }
                }
                completion(juce::var());
              });

  webComponent = std::make_unique<juce::WebBrowserComponent>(options);
  addAndMakeVisible(*webComponent);
  juce::URL url(uiFile.getFullPathName());
  webComponent->goToURL(url.toString(false));
  setSize(1000, 680);
  // Bind parameters to UI
  juce::StringArray allParams = {
      "input_drive", "tonic_note",    "tune_speed", "harmony_randomize",
      "mix_dry",     "mix_tuned",     "mix_tonic",  "mix_harmony",
      "pitch_key",   "pitch_scale",   "fx_enable",  "fx_glimmer",
      "fx_hf_grit",  "master_dry_wet"};
  for (const auto &p : allParams)
    bindParameter(p);
}

MessinaAudioProcessorEditor::~MessinaAudioProcessorEditor() {
  juce::StringArray allParams = {
      "input_drive", "tonic_note",    "tune_speed", "harmony_randomize",
      "mix_dry",     "mix_tuned",     "mix_tonic",  "mix_harmony",
      "pitch_key",   "pitch_scale",   "fx_enable",  "fx_glimmer",
      "fx_hf_grit",  "master_dry_wet"};
  for (const auto &p : allParams)
    audioProcessor.apvts.removeParameterListener(p, this);
}

//==============================================================================
void MessinaAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void MessinaAudioProcessorEditor::resized() {
  if (webComponent != nullptr)
    webComponent->setBounds(getLocalBounds());
}

void MessinaAudioProcessorEditor::bindParameter(const juce::String &paramID) {
  audioProcessor.apvts.addParameterListener(paramID, this);
}

void MessinaAudioProcessorEditor::parameterChanged(
    const juce::String &parameterID, float newValue) {
  if (webComponent != nullptr) {
    juce::MessageManager::callAsync([this, parameterID, newValue]() {
      // Update web UI when APVTS parameter changes
      juce::var data(new juce::DynamicObject());
      data.getDynamicObject()->setProperty("paramId", parameterID);

      float normalized = newValue;
      if (auto *param = audioProcessor.apvts.getParameter(parameterID)) {
        normalized = param->convertTo0to1(newValue);
      }

      data.getDynamicObject()->setProperty("value", newValue);

      webComponent->emitEventIfBrowserIsVisible("juceUpdate", data);
    });
  }
}
