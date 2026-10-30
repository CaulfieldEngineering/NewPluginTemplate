/*
  ==============================================================================

    PluginTests.cpp
    Ported from Tertiary's TertiaryTests.cpp by Claude, 2026-10-29

    Unit tests on the plugin's processor, with JUCE's own UnitTest class (no extra
    library). Built only with -DWLA_TESTS=ON; the program is named <wPLUGIN_NAME>Tests:

        cmake -B build-tests -DWLA_TESTS=ON
        cmake --build build-tests --config Release --target <wPLUGIN_NAME>Tests
        <wPLUGIN_NAME>Tests          (prints each test; exit code 0 when all pass)

    The release pipeline (.github/workflows/workflow.yml) runs this on the build it signs,
    and test.yml runs it on every push to a work branch.

    GENERIC tests (keep them; they work for any plugin made from the template)
      - processes at 44.1, 48 and 96 kHz and at odd block sizes, output finite
      - no memory allocation in processBlock
      - the same output at any block size
      - state save and restore (and a damaged state is ignored)
      - the demo cycle: on 50 s, off 10 s, and activating ends it

    PRODUCT tests: fill in ProductTests at the bottom with this plugin's own checks
    (Tertiary's LFO, tempo and licence tests are listed there as examples).

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Allocation counter: every new/delete in this program goes through here. While
// countAllocations is true on a thread, allocations on that thread are counted.
//==============================================================================
namespace
{
    thread_local bool countAllocations = false;
    std::atomic<int> allocationCount { 0 };
}

void* operator new (std::size_t size)
{
    if (countAllocations)
        ++allocationCount;

    if (auto* p = std::malloc (size == 0 ? 1 : size))
        return p;

    throw std::bad_alloc();
}

void* operator new[] (std::size_t size)                         { return operator new (size); }
void operator delete (void* p) noexcept                         { std::free (p); }
void operator delete[] (void* p) noexcept                       { std::free (p); }
void operator delete (void* p, std::size_t) noexcept            { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept          { std::free (p); }

namespace
{
using Processor = audio_plugin::AudioPluginAudioProcessor;
const juce::String testCategory { JucePlugin_Name };

/** A host clock at a settable tempo, playing from the top */
struct TestPlayHead : juce::AudioPlayHead
{
    double bpm = 120, rate = 48000;
    juce::int64 sample = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setTimeSignature (TimeSignature { 4, 4 });
        p.setIsPlaying (true);
        p.setTimeInSamples (sample);
        p.setPpqPosition (sample / rate * bpm / 60.0);
        return p;
    }
};

/** Sets a parameter by its ID, in the parameter's own units (dB, Hz, a choice's index) */
bool setParam (juce::AudioProcessor& p, const juce::String& id, float value)
{
    for (auto* prm : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (r->getParameterID() == id)
            {
                r->setValueNotifyingHost (r->convertTo0to1 (value));
                return true;
            }
    return false;
}

/** Noise in, n samples at a time; returns the left channel of the output */
std::vector<float> renderNoise (Processor& p, std::initializer_list<int> sizes, int total, int seed = 7)
{
    const int maxBlock = std::max (sizes);
    juce::AudioBuffer<float> buffer (2, maxBlock), input (2, total + maxBlock);
    juce::MidiBuffer midi;
    juce::Random rnd (seed);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < input.getNumSamples(); ++i)
            input.setSample (c, i, rnd.nextFloat() - 0.5f);

    std::vector<float> out;
    std::vector<int> pattern (sizes);
    for (int pos = 0, k = 0; pos < total; ++k)
    {
        const int n = pattern[(size_t) k % pattern.size()];
        juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, n);
        for (int c = 0; c < 2; ++c)
            view.copyFrom (c, 0, input, c, pos, n);
        p.processBlock (view, midi);
        out.insert (out.end(), view.getReadPointer (0), view.getReadPointer (0) + n);
        pos += n;
    }
    out.resize ((size_t) total);
    return out;
}

void runDC (Processor& p, int samples, int block)
{
    juce::AudioBuffer<float> b (2, block);
    juce::MidiBuffer midi;
    for (int done = 0; done < samples; done += block)
    {
        for (int c = 0; c < 2; ++c)
            juce::FloatVectorOperations::fill (b.getWritePointer (c), 0.5f, block);
        p.processBlock (b, midi);
    }
}
} // namespace


//==============================================================================
// GENERIC: keep these for every plugin
//==============================================================================
struct ProcessorTests : juce::UnitTest
{
    ProcessorTests() : juce::UnitTest ("Processor (generic)", testCategory) {}

    void runTest() override
    {
        beginTest ("Processes at 44.1, 48 and 96 kHz and odd block sizes; output finite");
        {
            for (double rate : { 44100.0, 48000.0, 96000.0 })
            {
                Processor p;
                p.setRateAndBufferSizeDetails (rate, 2048);
                p.prepareToPlay (rate, 2048);
                auto out = renderNoise (p, { 1, 32, 2048, 333, 7, 1024 }, (int) rate * 2);
                int bad = 0;
                for (auto v : out)
                    bad += std::isfinite (v) && std::abs (v) < 16.0f ? 0 : 1;
                expectEquals (bad, 0, "non-finite or huge samples at " + juce::String (rate));
                p.releaseResources();
            }
        }

        beginTest ("No allocation in processBlock (changing block sizes and tempo)");
        {
            Processor p;
            TestPlayHead head;
            p.setPlayHead (&head);
            p.setDemoState (false);
            p.setRateAndBufferSizeDetails (48000, 1024);
            p.prepareToPlay (48000, 1024);
            juce::AudioBuffer<float> buffer (2, 1024);
            juce::MidiBuffer midi;
            juce::Random rnd (42);

            auto process = [&] (int n)
            {
                juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, n);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                        view.setSample (c, i, rnd.nextFloat() - 0.5f);
                countAllocations = true;
                p.processBlock (view, midi);
                countAllocations = false;
                head.sample += n;
            };

            process (1024);   // a first block may set things up; it is not counted below
            allocationCount = 0;
            for (int n : { 1024, 1, 7, 64, 333, 512, 1024, 17, 256, 1000, 3 })
                process (n);
            head.bpm = 97;
            for (int n : { 128, 1024, 5 })
                process (n);
            // JUCE's buffers and arrays allocate with malloc, which this counter can't see: check any buffer
            // of your own is sized in prepareToPlay (a ProductTests check can compare its data pointer).
            expectEquals (allocationCount.load(), 0);
            p.setPlayHead (nullptr);
        }

        beginTest ("Same output at any block size");
        {
            auto render = [] (std::initializer_list<int> sizes)
            {
                Processor p;
                p.setDemoState (false);
                p.setRateAndBufferSizeDetails (48000, 1024);
                p.prepareToPlay (48000, 1024);
                return renderNoise (p, sizes, 48000 * 3);
            };

            auto a = render ({ 512 });
            auto b = render ({ 1, 1000, 64, 333, 7, 1024 });
            float maxDiff = 0;
            for (size_t i = 0; i < a.size(); ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (a[i] - b[i]));
            logMessage ("max difference " + juce::String (maxDiff));
            expectLessThan (maxDiff, 1.0e-6f);
        }

        beginTest ("State save and restore; a damaged state is ignored");
        {
            Processor a, b;
            juce::Random rnd (99);
            for (auto* prm : a.getParameters())
                prm->setValueNotifyingHost (rnd.nextFloat());
            juce::MemoryBlock state;
            a.getStateInformation (state);
            b.setStateInformation (state.getData(), (int) state.getSize());

            auto pa = a.getParameters(), pb = b.getParameters();
            expectEquals (pa.size(), pb.size());
            for (int i = 0; i < pa.size(); ++i)
                expectWithinAbsoluteError (pb[i]->getValue(), pa[i]->getValue(), 1.0e-6f, pa[i]->getName (64));

            b.setStateInformation ("garbage", 7);   // a bad state is ignored, not a crash
            b.setStateInformation (nullptr, 0);
            logMessage (juce::String (pa.size()) + " parameters");
        }

        beginTest ("Demo cycle: on 50 s, off 10 s; activating during the gap turns it back on");
        {
            Processor p;
            p.setRateAndBufferSizeDetails (48000, 480);
            p.prepareToPlay (48000, 480);
            p.setDemoState (true);
            runDC (p, 48000 * 49, 480);
            expect (p.isEffectEnabled(), "on at 49 s");
            runDC (p, 48000 * 2, 480);              // 51 s
            expect (! p.isEffectEnabled(), "off at 51 s");
            runDC (p, 48000 * 10, 480);             // 61 s
            expect (p.isEffectEnabled(), "on again at 61 s");
            runDC (p, 48000 * 50, 480);             // 111 s, in the next gap
            expect (! p.isEffectEnabled(), "off at 111 s");

            p.setDemoState (false);                 // the user activates now
            runDC (p, 480, 480);
            expect (p.isEffectEnabled(), "on once activated");
            runDC (p, 48000 * 70, 480);
            expect (p.isEffectEnabled(), "and stays on");
        }
    }
};


//==============================================================================
// PRODUCT: this plugin's own tests. Fill in; keep the category so they run.
//
// Examples from Tertiary (plugin/tests/TertiaryTests.cpp in the Tertiary repo):
//   - LFO speed is right after a sample-rate change (DC in, measure the period of the output)
//   - a synced LFO follows a host tempo change (TestPlayHead above, change head.bpm mid-run)
//   - relative phase in the audio matches the knob
//   - the licence re-check, against a pretend server (tests/PretendServer.h there): never the live server
//   - factory presets: names unique, every parameter name real, first is Default
// Each was written after a bug or a change; a bug Joe finds gets its check here (or in HostTest) first.
//==============================================================================
struct ProductTests : juce::UnitTest
{
    ProductTests() : juce::UnitTest ("Product", testCategory) {}

    void runTest() override
    {
        beginTest ("Product checks (none yet: add this plugin's own here)");
        {
            Processor p;
            p.setRateAndBufferSizeDetails (48000, 512);
            p.prepareToPlay (48000, 512);
            // Example shape:
            //   expect (setParam (p, "Rate", 2.0f), "parameter Rate exists");
            //   runDC (p, 48000 * 2, 512);
            //   expectWithinAbsoluteError (measuredPeriod, 24000.0, 240.0);
            juce::ignoreUnused (&setParam);
            expect (true);
        }
    }
};


static ProcessorTests processorTests;
static ProductTests productTests;


/* Prints the test log to the console (JUCE writes it to the debugger by default) */
struct ConsoleLogger : juce::Logger
{
    void logMessage (const juce::String& message) override    { std::cout << message << std::endl; }
};

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger (&logger);

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory (testCategory);

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED: " + std::to_string (failures)) << std::endl;
    juce::Logger::setCurrentLogger (nullptr);
    return failures == 0 ? 0 : 1;
}
