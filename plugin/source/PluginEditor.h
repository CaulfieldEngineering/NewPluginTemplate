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

    private:

        // Members
        // ==============================================================================

        AudioPluginAudioProcessor &processorRef;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
    };

} // namespace audio_plugin
