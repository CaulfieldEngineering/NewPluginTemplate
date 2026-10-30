/*
  ==============================================================================

    HostTest.cpp
    Ported from Tertiary's HostTest.cpp by Claude, 2026-10-29

    A small host: it loads the BUILT plugin (the .vst3, or the installed AU on a Mac) the way
    a DAW does, through juce::AudioPluginFormatManager, and runs the manual checklist as
    automated checks. Every check prints one line: PASS, FAIL, SKIP or INFO.

        HostTest --vst3 <path to the .vst3>          (Windows and Mac)
        HostTest --au                                 (Mac: the AU installed in Components, found by
                                                       the codes in config.cmake)
        options:  --golden <folder>   reference renders (default plugin/tests/golden)
                  --make-golden       write the reference renders instead of comparing
                  --expect-demo       this machine has no key, so the demo gaps must be there
                                      (checked by the product section; see section 7)
                  --only <n>          run one section (1 to 7)
        HostTest --write-input <file.wav>             the golden renders' input, for the REAPER test

    Exit code 0 when nothing FAILed.

    GENERIC sections (keep them; they work for any plugin made from the template)
      1  Load, prepare, process at 44.1/48/96 kHz, blocks 32 to 2048, changing mid-run
      2  Automation of every automatable parameter through the host's parameter API
      3  Session save and restore (exact values, same bytes, bit-identical render)
      4  Host bypass passes the audio through untouched
      5  Golden renders: fixed input and settings, compared with files in the repo
    PRODUCT sections (fill in for this plugin)
      6  goldenSettings(): add settings worth a reference render each (section 5 runs them)
      7  productChecks(): this plugin's own checks. Tertiary's are listed there as examples
         (tempo-synced LFO, band mute/solo, the demo gap, licence states, presets, old sessions).

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#ifndef WLA_AU_ID
    #define WLA_AU_ID ""   // ",<plugin code>,<manufacturer code>", set by plugin/CMakeLists.txt from config.cmake
#endif

namespace
{
//==============================================================================
// Results
//==============================================================================
int passes = 0, failures = 0, skips = 0;

void report(const juce::String& status, const juce::String& name, const juce::String& detail = {})
{
    if (status == "PASS") ++passes;
    if (status == "FAIL") ++failures;
    if (status == "SKIP") ++skips;
    std::cout << status.paddedRight(' ', 5) << " " << name.paddedRight(' ', 58) << " " << detail << std::endl;
}

void check(bool ok, const juce::String& name, const juce::String& detail = {})
{
    report(ok ? "PASS" : "FAIL", name, detail);
}

//==============================================================================
// Loading the plugin
//==============================================================================
juce::AudioPluginFormatManager formatManager;
juce::PluginDescription description;

bool findPlugin(const juce::StringArray& args)
{
    formatManager.addDefaultFormats();

    juce::String formatName, fileOrId;

    if (args.contains("--vst3"))
    {
        formatName = "VST3";
        fileOrId = juce::File::getCurrentWorkingDirectory().getChildFile(args[args.indexOf("--vst3") + 1]).getFullPathName();
    }
    else if (args.contains("--au"))
    {
        formatName = "AudioUnit";
    }

    for (auto* format : formatManager.getFormats())
    {
        if (format->getName() != formatName)
            continue;

        juce::StringArray candidates;
        if (fileOrId.isNotEmpty())
            candidates.add(fileOrId);
        else
            candidates = format->searchPathsForPlugins({}, true, false);   // AU: every installed component

        for (auto& candidate : candidates)
        {
            if (formatName == "AudioUnit" && ! candidate.contains(WLA_AU_ID))
                continue;

            juce::OwnedArray<juce::PluginDescription> found;
            format->findAllTypesForFile(found, candidate);

            if (! found.isEmpty())
            {
                description = *found[0];
                return true;
            }
        }
    }
    return false;
}

std::unique_ptr<juce::AudioPluginInstance> create(double rate, int maxBlock)
{
    juce::String error;
    auto instance = formatManager.createPluginInstance(description, rate, maxBlock, error);

    if (instance == nullptr)
    {
        report("FAIL", "create instance", error);
        return nullptr;
    }

    juce::AudioProcessor::BusesLayout stereo;
    stereo.inputBuses.add(juce::AudioChannelSet::stereo());
    stereo.outputBuses.add(juce::AudioChannelSet::stereo());
    instance->setBusesLayout(stereo);
    instance->setRateAndBufferSizeDetails(rate, maxBlock);
    instance->prepareToPlay(rate, maxBlock);
    return instance;
}

[[maybe_unused]] juce::AudioProcessorParameter* findParam(juce::AudioPluginInstance& p, const juce::String& name)
{
    for (auto* prm : p.getParameters())
        if (prm->getName(256) == name)
            return prm;
    return nullptr;
}

/* Sets a parameter by its name, from its normalised (0 to 1) value, as a host's automation does */
[[maybe_unused]] void setNorm(juce::AudioPluginInstance& p, const juce::String& name, float value)
{
    if (auto* prm = findParam(p, name))
        prm->setValueNotifyingHost(value);
    else
        report("FAIL", "parameter exists: " + name);
}

//==============================================================================
// Signals and rendering
//==============================================================================
juce::AudioBuffer<float> sine(double rate, double seconds, float freq = 220.0f, float amplitude = 0.25f)
{
    juce::AudioBuffer<float> b(2, (int) (rate * seconds));
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        auto v = amplitude * (float) std::sin(juce::MathConstants<double>::twoPi * freq * i / rate);
        b.setSample(0, i, v);
        b.setSample(1, i, v);
    }
    return b;
}

/* The golden-render input: soft noise, a sweep and a click every 0.25 s. The same on every machine. */
juce::AudioBuffer<float> testSignal(double rate, double seconds)
{
    juce::AudioBuffer<float> b(2, (int) (rate * seconds));
    juce::Random rnd(1234);
    double phase = 0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double f = 40.0 * std::pow(250.0, i / (double) b.getNumSamples());   // 40 Hz to 10 kHz
        phase += juce::MathConstants<double>::twoPi * f / rate;
        const float click = (i % (int) (rate / 4) == 0) ? 0.5f : 0.0f;
        b.setSample(0, i, 0.25f * (float) std::sin(phase) + 0.05f * (rnd.nextFloat() * 2 - 1) + click);
        b.setSample(1, i, 0.20f * (float) std::sin(phase) + 0.05f * (rnd.nextFloat() * 2 - 1) - click);
    }
    return b;
}

using BlockCallback = std::function<void(juce::AudioPluginInstance&, int position)>;

/* Runs input through the plugin in blocks (sizes cycle through the list); returns the output */
juce::AudioBuffer<float> render(juce::AudioPluginInstance& p, const juce::AudioBuffer<float>& input,
                                std::vector<int> blocks = { 512 }, BlockCallback beforeBlock = {})
{
    const int maxBlock = *std::max_element(blocks.begin(), blocks.end());
    juce::AudioBuffer<float> out(2, input.getNumSamples()), io(2, maxBlock);
    juce::MidiBuffer midi;

    for (int pos = 0, k = 0; pos < input.getNumSamples(); ++k)
    {
        const int n = juce::jmin(blocks[(size_t) k % blocks.size()], input.getNumSamples() - pos);
        if (beforeBlock)
            beforeBlock(p, pos);
        juce::AudioBuffer<float> view(io.getArrayOfWritePointers(), 2, n);
        for (int c = 0; c < 2; ++c)
            view.copyFrom(c, 0, input, c, pos, n);
        p.processBlock(view, midi);
        for (int c = 0; c < 2; ++c)
            out.copyFrom(c, pos, view, c, 0, n);
        pos += n;
    }
    return out;
}

/* A fresh instance, set up, a moment of silence so the settings reach the plugin, then the input */
juce::AudioBuffer<float> renderWith(std::function<void(juce::AudioPluginInstance&)> setUp, const juce::AudioBuffer<float>& input)
{
    auto p = create(48000, 512);
    if (p == nullptr) return {};
    setUp(*p);
    juce::AudioBuffer<float> silence(2, 256);
    silence.clear();
    render(*p, silence);
    return render(*p, input);
}

struct Stats { float peak = 0, rms = 0, maxJump = 0; int nonFinite = 0, subnormal = 0; };

Stats stats(const juce::AudioBuffer<float>& b, int from = 0)
{
    Stats s;
    double sum = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        auto* x = b.getReadPointer(c);
        for (int i = from; i < b.getNumSamples(); ++i)
        {
            if (! std::isfinite(x[i])) { ++s.nonFinite; continue; }
            if (x[i] != 0 && std::abs(x[i]) < std::numeric_limits<float>::min()) ++s.subnormal;
            s.peak = juce::jmax(s.peak, std::abs(x[i]));
            sum += x[i] * x[i];
            if (i > from) s.maxJump = juce::jmax(s.maxJump, std::abs(x[i] - x[i - 1]));
        }
    }
    s.rms = (float) std::sqrt(sum / juce::jmax(1, b.getNumChannels() * (b.getNumSamples() - from)));
    return s;
}

float maxDiff(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < juce::jmin(a.getNumSamples(), b.getNumSamples()); ++i)
            d = juce::jmax(d, std::abs(a.getSample(c, i) - b.getSample(c, i)));
    return d;
}

juce::String dB(float x) { return juce::String(juce::Decibels::gainToDecibels(x, -300.0f), 1) + " dBFS"; }

//==============================================================================
// 1. Load, prepare, process
//==============================================================================
void section1()
{
    const std::vector<int> blocks { 32, 64, 128, 256, 512, 1024, 2048, 1000, 333, 37 };

    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        auto p = create(rate, 2048);
        if (p == nullptr) return;
        // Silence after the tone, longer than the plugin's own tail, must end in silence
        const double tailSeconds = juce::jmin(10.0, p->getTailLengthSeconds()) + 1.0;
        auto in = sine(rate, 3);
        juce::AudioBuffer<float> withSilence(2, in.getNumSamples() + (int) (rate * tailSeconds));
        withSilence.clear();
        for (int c = 0; c < 2; ++c) withSilence.copyFrom(c, 0, in, c, 0, in.getNumSamples());

        auto out = render(*p, withSilence, blocks);
        auto s = stats(out), si = stats(in);
        auto tail = stats(out, out.getNumSamples() - (int) (rate * 0.1));
        auto name = "1 process at " + juce::String(rate / 1000.0, 1) + " kHz, blocks 32-2048";
        check(s.nonFinite == 0 && s.subnormal == 0 && s.peak < 4.0f && tail.peak < 1.0e-4f,
              name, "peak " + dB(s.peak) + " (input " + dB(si.peak) + "), NaN/inf " + juce::String(s.nonFinite)
                  + ", denormals " + juce::String(s.subnormal) + ", end of tail " + dB(tail.peak));
    }

    // Change sample rate and block size on a running instance, as a host does when the project changes
    {
        auto p = create(44100, 512);
        if (p == nullptr) return;
        render(*p, sine(44100, 1), { 512 });
        p->releaseResources();
        p->setRateAndBufferSizeDetails(96000, 2048);
        p->prepareToPlay(96000, 2048);
        auto s = stats(render(*p, sine(96000, 2), { 2048, 31, 700 }));
        check(s.nonFinite == 0 && s.peak < 4.0f, "1 change 44.1 to 96 kHz mid-run",
              "peak " + dB(s.peak) + ", NaN/inf " + juce::String(s.nonFinite));
    }

    {
        auto p = create(48000, 512);
        if (p == nullptr) return;
        report("INFO", "1 reported latency and tail", juce::String(p->getLatencySamples()) + " samples, tail "
               + juce::String(p->getTailLengthSeconds(), 2) + " s, " + juce::String(p->getParameters().size()) + " parameters");
    }
}

//==============================================================================
// 2. Automation: every automatable parameter ramped 0 to 1 over the render, like host automation
//==============================================================================
void section2()
{
    const double rate = 48000;
    auto input = testSignal(rate, 2);

    juce::AudioBuffer<float> reference;
    int numParams = 0;
    {
        auto p = create(rate, 512);
        if (p == nullptr) return;
        numParams = p->getParameters().size();
        reference = render(*p, input, { 64 });
    }
    if (numParams == 0)
        report("SKIP", "2 automation", "the plugin has no parameters yet");

    for (int index = 0; index < numParams; ++index)
    {
        auto p = create(rate, 512);
        if (p == nullptr) return;
        auto* prm = p->getParameters()[index];
        if (! prm->isAutomatable())
            continue;

        const auto name = prm->getName(256);
        const int total = input.getNumSamples();
        auto out = render(*p, input, { 64 }, [prm, total](juce::AudioPluginInstance&, int pos) {
            prm->setValueNotifyingHost((float) pos / (float) total);
        });

        auto s = stats(out);
        // Whether a parameter SHOULD change the sound, and how smoothly, is the product's to judge (section 7;
        // Tertiary checks smooth parameters make no clicks). Here: no NaN, no blow-up, and what it changed.
        check(s.nonFinite == 0 && s.peak < 16.0f, "2 automate " + name,
              "peak " + dB(s.peak) + ", largest step " + juce::String(s.maxJump, 4)
              + ", changed the sound by up to " + juce::String(maxDiff(out, reference), 4));
    }
}

//==============================================================================
// 3. Session save and restore
//==============================================================================
void section3()
{
    const double rate = 48000;
    auto input = testSignal(rate, 2);
    juce::Random rnd(99);
    juce::MemoryBlock state;
    juce::Array<float> values;
    juce::AudioBuffer<float> first, second;
    juce::AudioBuffer<float> silence(2, 256);
    silence.clear();

    // A session with random settings
    juce::MemoryBlock session;
    {
        auto a = create(rate, 512);
        if (a == nullptr) return;
        for (auto* prm : a->getParameters())
            if (prm != a->getBypassParameter() && prm->getName(256) != "Program")   // not the host's preset selector
                prm->setValueNotifyingHost(rnd.nextFloat());
        render(*a, silence, { 256 });                 // let the changes reach the plugin
        a->getStateInformation(session);
    }

    // The session opened: its values, its sound, and its state saved again. (The host's own copy of values it
    // sent isn't rounded to the plugin's steps, so the values are read from an instance that loaded the session.)
    {
        auto p = create(rate, 512);
        if (p == nullptr) return;
        p->setStateInformation(session.getData(), (int) session.getSize());
        render(*p, silence, { 256 });
        for (auto* prm : p->getParameters())
            values.add(prm->getValue());
        p->getStateInformation(state);
        first = render(*p, input, { 512 });
    }   // destroyed, as when the project is closed

    auto b = create(rate, 512);
    if (b == nullptr) return;
    b->setStateInformation(state.getData(), (int) state.getSize());
    render(*b, silence, { 256 });
    int mismatches = 0;
    juce::String firstMismatch;
    for (int i = 0; i < b->getParameters().size(); ++i)
        if (b->getParameters()[i]->getValue() != values[i])
        {
            if (mismatches++ == 0)
                firstMismatch = b->getParameters()[i]->getName(64) + " " + juce::String(values[i]) + " -> " + juce::String(b->getParameters()[i]->getValue());
        }
    check(mismatches == 0, "3 restore: every parameter exactly the same",
          juce::String(values.size()) + " parameters, " + juce::String(mismatches) + " differ " + firstMismatch);

    juce::MemoryBlock resaved;
    b->getStateInformation(resaved);
    check(resaved == state, "3 restore: saving again gives the same bytes", juce::String((int) state.getSize()) + " bytes");

    second = render(*b, input, { 512 });
    auto d = maxDiff(first, second);
    check(d == 0.0f, "3 restore: render bit-identical to before the save", d == 0 ? "identical" : "max diff " + juce::String(d));
}

//==============================================================================
// 4. Host bypass
//==============================================================================
void section4()
{
    auto input = testSignal(48000, 2);
    int latency = 0;
    bool hasBypass = false;
    auto bypassed = renderWith([&](juce::AudioPluginInstance& p) {
        latency = p.getLatencySamples();
        if (auto* b = p.getBypassParameter()) { b->setValueNotifyingHost(1.0f); hasBypass = true; }
    }, input);

    if (! hasBypass) { report("SKIP", "4 host bypass", "the plugin has no bypass parameter"); return; }
    if (latency != 0) { report("SKIP", "4 host bypass passes the audio through", "latency " + juce::String(latency) + ": compare by hand"); return; }
    auto d = maxDiff(bypassed, input);
    check(d < 1.0e-7f, "4 host bypass passes the audio through untouched", "max diff " + juce::String(d));
}

//==============================================================================
// 5 and 6. Golden renders
//==============================================================================
struct GoldenSetting
{
    juce::String name;
    std::function<void(juce::AudioPluginInstance&)> setUp;
};

// 6. PRODUCT: add settings worth a reference render each. "default" stays first (the REAPER test compares with it).
// Example from Tertiary:
//     { "varied", [](juce::AudioPluginInstance& p) { setNorm(p, "Depth Mid Band", 0.6f); setNorm(p, "Invert Low Band", 1.0f); } },
std::vector<GoldenSetting> goldenSettings()
{
    return {
        { "default", [](juce::AudioPluginInstance&) {} },
    };
}

float goldenTolerance()
{
   #if JUCE_ARM
    return 1.0e-5f;     // -100 dBFS: Apple Silicon rounds differently from the x64 machine that made the files
   #else
    return 1.0e-6f;     // -120 dBFS
   #endif
}

void section5(const juce::File& folder, bool make)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto input = testSignal(48000, 2);

    for (auto& setting : goldenSettings())
    {
        auto out = renderWith(setting.setUp, input);
        auto file = folder.getChildFile(setting.name + ".wav");

        if (make)
        {
            folder.createDirectory();
            file.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(new juce::FileOutputStream(file), 48000, 2, 32, {}, 0));
            writer->writeFromAudioSampleBuffer(out, 0, out.getNumSamples());
            report("INFO", "5 golden " + setting.name, "written to " + file.getFullPathName());
            continue;
        }

        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if (reader == nullptr)
        {
            check(false, "5 golden " + setting.name, "reference missing: " + file.getFullPathName() + " (HostTest --make-golden)");
            continue;
        }
        juce::AudioBuffer<float> golden(2, (int) reader->lengthInSamples);
        reader->read(&golden, 0, golden.getNumSamples(), 0, true, true);
        auto d = maxDiff(out, golden);
        check(golden.getNumSamples() == out.getNumSamples() && d <= goldenTolerance(), "5 golden " + setting.name,
              (d == 0 ? juce::String("identical") : "max diff " + juce::String(d) + " (" + dB(d) + ")")
              + ", tolerance " + juce::String(goldenTolerance()));
    }
}

//==============================================================================
// 7. PRODUCT: this plugin's own checks through the host
//
// Examples from Tertiary (its HostTest.cpp, sections 4 to 9):
//   - tempo: a synced LFO's period follows the host's tempo (a Clock playhead at 60/120/150 bpm, DC input)
//   - band mute, solo and bypass agree with the crossover
//   - demo mode (expectDemo): 250 ms windows over 60 s; the effect is off 10 s in every 60 s on an unlicensed copy
//   - licence states with a pretend server, in this program, never the live server
//   - presets through the host's program list; a session saved by the previous release loads unchanged
//     (golden/session-<version>-VST3.bin, written with --write-session from the old build)
// A bug Joe finds gets a check here or in the unit tests first, then the fix.
//==============================================================================
void productChecks(bool expectDemo)
{
    if (expectDemo)
        report("SKIP", "7 demo gaps", "no product check yet: write one for this plugin's effect (see Tertiary's section 6)");
    report("SKIP", "7 product checks", "none yet (plugin/tests/HostTest.cpp, productChecks)");
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add(argv[i]);

    // The golden renders' input as a WAV, for the REAPER test (tests/reaper)
    if (args.contains("--write-input"))
    {
        const juce::File file(juce::File::getCurrentWorkingDirectory().getChildFile(args[args.indexOf("--write-input") + 1]));
        file.deleteFile();
        auto input = testSignal(48000, 2);
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(new juce::FileOutputStream(file), 48000, 2, 32, {}, 0));
        writer->writeFromAudioSampleBuffer(input, 0, input.getNumSamples());
        std::cout << "wrote " << file.getFullPathName() << std::endl;
        return 0;
    }

    if (! args.contains("--vst3") && ! args.contains("--au"))
    {
        std::cout << "usage: HostTest --vst3 <plugin.vst3> | --au  [--golden <folder>] [--make-golden] [--expect-demo] [--only n]" << std::endl;
        return 2;
    }

    const auto start = juce::Time::getMillisecondCounterHiRes();

    if (! findPlugin(args))
    {
        report("FAIL", "find the plugin", args.contains("--au") ? juce::String("no AU with codes ") + WLA_AU_ID + " installed"
                                                                : "can't load " + args[args.indexOf("--vst3") + 1]);
        return 1;
    }
    report("PASS", "load " + description.pluginFormatName + " " + description.name + " " + description.version,
           description.fileOrIdentifier);

    auto option = [&](const juce::String& name, const juce::String& fallback) {
        const int i = args.indexOf(name);
        return i >= 0 && i + 1 < args.size() ? args[i + 1] : fallback;
    };
    const int only = option("--only", "0").getIntValue();
    const juce::File golden = juce::File::getCurrentWorkingDirectory().getChildFile(option("--golden", "plugin/tests/golden"));
    const bool makeGolden = args.contains("--make-golden");

    auto run = [&](int n, std::function<void()> section) {
        if (only != 0 && only != n) return;
        const auto t = juce::Time::getMillisecondCounterHiRes();
        section();
        std::cout << "      section " << n << " took " << juce::String((juce::Time::getMillisecondCounterHiRes() - t) / 1000.0, 1) << " s" << std::endl;
    };

    if (! makeGolden)
    {
        run(1, section1);
        run(2, section2);
        run(3, section3);
        run(4, section4);
        run(7, [&] { productChecks(args.contains("--expect-demo")); });
    }
    run(5, [&] { section5(golden, makeGolden); });

    std::cout << "HostTest " << description.pluginFormatName << ": " << passes << " passed, " << failures << " failed, "
              << skips << " skipped, " << juce::String((juce::Time::getMillisecondCounterHiRes() - start) / 1000.0, 1) << " s" << std::endl;
    return failures == 0 ? 0 : 1;
}
