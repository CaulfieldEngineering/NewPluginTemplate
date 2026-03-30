#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace audio_plugin {
    AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
        AudioPluginAudioProcessor &p)
        : AudioProcessorEditor(&p), processorRef(p) {
    juce::ignoreUnused(processorRef);
    // Editor size must be at least 750x515 for Wonderlab web builds.
    // The web container defaults to 750x515, and the canvas detection
    // requires width >= 700. Smaller sizes will render incorrectly.
    setSize(750, 515);
    }

    AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

    void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

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

    void AudioPluginAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    }
} // namespace audio_plugin