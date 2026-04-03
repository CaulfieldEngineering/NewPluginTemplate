#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace audio_plugin {

    // Constructor
    // ==============================================================================

    AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
        AudioPluginAudioProcessor &p)
        : AudioProcessorEditor(&p), processorRef(p) {
    juce::ignoreUnused(processorRef);

    // Editor size must be at least 750x515 for Wonderlab web builds.
    setSize(750, 515);
    }

    // Destructor
    // ==============================================================================

    AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

    // paint
    // ==============================================================================

    void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::grey);

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);

    #ifdef DEMO_VERSION
        g.drawFittedText("Hello World! - Demo Version!", getLocalBounds(),
                        juce::Justification::centred, 1);
    #else
       g.drawFittedText("Hello World!", getLocalBounds(),
                        juce::Justification::centred, 1);
    #endif
    }

    // resized
    // ==============================================================================

    void AudioPluginAudioProcessorEditor::resized() {
    }

} // namespace audio_plugin
