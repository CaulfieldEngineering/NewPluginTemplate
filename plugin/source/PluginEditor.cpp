#include "PluginEditor.h"
#include "PluginProcessor.h"

// [Claude 2026-10-29] build_id.h is written before every build (plugin/cmake/build_id.cmake). Builds that don't make
// it, such as the Wonderlab web build, show "unknown".
#if __has_include("build_id.h")
    #include "build_id.h"
#endif
#ifndef WLA_BUILD_ID
    #define WLA_BUILD_ID "unknown"
#endif

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

    if (mShowBuildId) {
        g.setFont(12.0f);
        g.drawText(juce::String("build ") + WLA_BUILD_ID,
                   getLocalBounds().reduced(8), juce::Justification::bottomRight, false);
    }
    }

    // mouseDown: Ctrl+Shift+click toggles the build number
    // ==============================================================================

    void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent &e) {
    if (e.mods.isCtrlDown() && e.mods.isShiftDown()) {
        mShowBuildId = ! mShowBuildId;
        repaint();
    }
    }

    // resized
    // ==============================================================================

    void AudioPluginAudioProcessorEditor::resized() {
    }

} // namespace audio_plugin
