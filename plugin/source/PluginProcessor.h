#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {

    class AudioPluginAudioProcessor : public juce::AudioProcessor {

    public:

        AudioPluginAudioProcessor();
        ~AudioPluginAudioProcessor() override;

        // Audio Lifecycle
        // ==============================================================================

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
        using AudioProcessor::processBlock;

        // UI
        // ==============================================================================

        juce::AudioProcessorEditor *createEditor() override;
        bool hasEditor() const override;

        // Host Info
        // ==============================================================================

        const juce::String getName() const override;
        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;

        // Programs
        // ==============================================================================

        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int index) override;
        const juce::String getProgramName(int index) override;
        void changeProgramName(int index, const juce::String &newName) override;

        // State
        // ==============================================================================

        void getStateInformation(juce::MemoryBlock &destData) override;
        void setStateInformation(const void *data, int sizeInBytes) override;

        // Demo / Activation
        // ==============================================================================

        bool getDemoState() const { return mIsDemoVersion; }
        void setDemoState(bool demoState);
        bool isEffectEnabled() const { return mEffectEnabled; }

    private:

        // Parameter Tree
        // ==============================================================================

        juce::AudioProcessorValueTreeState mParameterTree;
        juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
        juce::AudioProcessorValueTreeState& getParameterTree() { return mParameterTree; }

        // Demo State
        // ==============================================================================

        bool mIsDemoVersion { true };
        bool mEffectEnabled { true };
        int mDemoActiveSeconds { 50 };
        int mDemoBypassSeconds { 10 };
        int64_t mSampleCounter { 0 };
        int64_t mSamplesUntilDisable { 0 };
        int64_t mSamplesUntilEnable { 0 };

        // DSP
        // ==============================================================================

        // TODO: Add your DSP members here

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
    };

} // namespace audio_plugin
