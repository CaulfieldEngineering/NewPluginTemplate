#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace audio_plugin {

    // Constructor
    // ==============================================================================

    AudioPluginAudioProcessor::AudioPluginAudioProcessor()
        : AudioProcessor(
            BusesProperties()
    #if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
    #endif
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
        ),
        mParameterTree(*this, nullptr, "Parameters", createParameterLayout()) {
    }

    // Destructor
    // ==============================================================================

    AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

    // getName
    // ==============================================================================

    const juce::String AudioPluginAudioProcessor::getName() const {
    return JucePlugin_Name;
    }

    // acceptsMidi
    // ==============================================================================

    bool AudioPluginAudioProcessor::acceptsMidi() const {
    #if JucePlugin_WantsMidiInput
    return true;
    #else
    return false;
    #endif
    }

    // producesMidi
    // ==============================================================================

    bool AudioPluginAudioProcessor::producesMidi() const {
    #if JucePlugin_ProducesMidiOutput
    return true;
    #else
    return false;
    #endif
    }

    // isMidiEffect
    // ==============================================================================

    bool AudioPluginAudioProcessor::isMidiEffect() const {
    #if JucePlugin_IsMidiEffect
    return true;
    #else
    return false;
    #endif
    }

    // getTailLengthSeconds
    // ==============================================================================

    double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

    // getNumPrograms
    // ==============================================================================

    int AudioPluginAudioProcessor::getNumPrograms() {
    return 1;
    }

    // getCurrentProgram
    // ==============================================================================

    int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }

    // setCurrentProgram
    // ==============================================================================

    void AudioPluginAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
    }

    // getProgramName
    // ==============================================================================

    const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
    }

    // changeProgramName
    // ==============================================================================

    void AudioPluginAudioProcessor::changeProgramName(int index,
                                                    const juce::String &newName) {
    juce::ignoreUnused(index, newName);
    }

    // prepareToPlay
    // ==============================================================================

    void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);

    if (mIsDemoVersion) {
        mSamplesUntilDisable = static_cast<int64_t>(mDemoActiveSeconds * sampleRate);
        mSamplesUntilEnable  = static_cast<int64_t>(mDemoBypassSeconds * sampleRate);
    }
    }

    // releaseResources
    // ==============================================================================

    void AudioPluginAudioProcessor::releaseResources() {
    }

    // isBusesLayoutSupported
    // ==============================================================================

    bool AudioPluginAudioProcessor::isBusesLayoutSupported(
        const BusesLayout &layouts) const {
    #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
    #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
    #endif
    }

    // processBlock
    // ==============================================================================

    void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                                juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Demo nag cycle
    if (mIsDemoVersion) {
        mSampleCounter += buffer.getNumSamples();

        if (mEffectEnabled) {
            if (mSampleCounter >= mSamplesUntilDisable) {
                mEffectEnabled = false;
                mSampleCounter = 0;
            }
        } else {
            if (mSampleCounter >= mSamplesUntilEnable) {
                mEffectEnabled = true;
                mSampleCounter = 0;
            }
        }
    }

    // DSP processing — gate on isEffectEnabled() for demo support
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto *channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
        // TODO: Add your DSP here
    }
    }

    // hasEditor
    // ==============================================================================

    bool AudioPluginAudioProcessor::hasEditor() const {
    	return true;
    }

    // createEditor
    // ==============================================================================

    juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() {
    	return new AudioPluginAudioProcessorEditor(*this);
    	// For prototyping with auto-sized generic UI, swap the lines above/below:
    	// auto* editor = new juce::GenericAudioProcessorEditor(*this);
    	// editor->setSize(500, std::max(200, (int)getParameters().size() * 40 + 40));
    	// return editor;
    }

    // getStateInformation
    // ==============================================================================

    void AudioPluginAudioProcessor::getStateInformation(
        juce::MemoryBlock &destData) {
    auto state = mParameterTree.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
    }

    // setStateInformation
    // ==============================================================================

    void AudioPluginAudioProcessor::setStateInformation(const void *data,
                                                        int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(mParameterTree.state.getType())) {
            mParameterTree.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
    }

    // setDemoState
    // ==============================================================================

    void AudioPluginAudioProcessor::setDemoState(bool demoState) {
        mIsDemoVersion = demoState;

        if (mIsDemoVersion) {
            mSamplesUntilDisable = static_cast<int64_t>(mDemoActiveSeconds * getSampleRate());
            mSamplesUntilEnable  = static_cast<int64_t>(mDemoBypassSeconds * getSampleRate());
        }

        mSampleCounter = 0;
        mEffectEnabled = true;
    }

    // createParameterLayout
    // ==============================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // TODO: Add your parameters here

        return { params.begin(), params.end() };
    }

} // namespace audio_plugin

// createPluginFilter
// ==============================================================================

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}
