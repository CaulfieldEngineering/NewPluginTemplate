# Golden renders

Reference renders that `HostTest` compares with (section 5): its fixed test signal (2 s, 48 kHz: noise, a sweep, clicks) through the plugin with each setting in `goldenSettings()` in `HostTest.cpp`.

- Write or refresh them: `HostTest --vst3 <path to the built .vst3> --make-golden` (from the repository root), then commit them. Only when a change to the sound is meant, and say so in the commit.
- Tolerance: 1e-6 on x64, 1e-5 on Apple Silicon (it rounds differently from the Windows machine that wrote them).
- `default.wav` here is the template's example plugin, which passes the audio through unchanged. A new plugin overwrites it on its first `--make-golden`.
- The REAPER test (`../reaper`) compares its render with `default.wav` too.
