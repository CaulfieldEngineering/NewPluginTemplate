#pragma once

#include "PluginProcessor.h"

namespace audio_plugin {

    class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor {

    public:

        explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
        ~AudioPluginAudioProcessorEditor() override;

        // Rendering
        // ==============================================================================

        void paint(juce::Graphics &) override;
        void resized() override;

        // [Claude 2026-10-29] Ctrl+Shift+click shows the hidden build number (commit and build time), so a tester
        // can confirm the exact build. Keep this in your own editor (Tertiary has it on its About window).
        void mouseDown(const juce::MouseEvent &) override;

    private:

        // Members
        // ==============================================================================

        AudioPluginAudioProcessor &processorRef;
        bool mShowBuildId { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
    };

} // namespace audio_plugin
