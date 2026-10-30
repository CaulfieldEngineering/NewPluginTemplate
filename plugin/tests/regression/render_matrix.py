# render_matrix.py
# Written by Claude (2026-10-27) for Tertiary 1.1; ported to the template 2026-10-29. Joe's request: "anything you touch ... regression tested".
#
# Renders a fixed set of inputs through ReviewRender with a fixed set of parameter settings, so the output of one
# build can be compared with another (compare_renders.py). Used locally (before/after each change) and in CI
# (.github/workflows/test.yml, to compare Windows with both Macs).
#
#   python render_matrix.py <ReviewRender executable> <input folder> <output folder> [--quick]
#
# Every .wav in the input folder is rendered with every setting below (--quick: the first 8 seconds, fewer settings).
# A synthetic test signal (noise, a sweep and clicks, 48 kHz stereo) is written to the temp folder and rendered too.
# Output files are 32-bit float WAVs named <input>__<setting>.wav.

import math, os, random, struct, subprocess, sys, tempfile

# The settings come from render_settings.json next to this file (the product fills it in; see the "_example" entry
# there, from Tertiary). Each setting: a name and ReviewRender arguments, values in the parameter's own units or a
# choice's text. A setting whose name starts with "varied_" gets the "varied" setting's arguments added after its
# own (so "varied_block512" is "varied" at another block size). "quick" lists the ones --quick renders.
import json
_cfg = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "render_settings.json")))
SETTINGS = [(n, a) for n, a in _cfg["settings"].items() if not n.startswith("_")]
VARIED = dict(SETTINGS).get("varied", [])
SETTINGS = [(n, a + VARIED if n.startswith("varied_") else a) for n, a in SETTINGS]
QUICK = _cfg.get("quick", [n for n, _ in SETTINGS])


def write_test_signal(path, rate=48000, seconds=20):
    """Deterministic stereo test signal: soft noise, a log sweep (20 Hz to 20 kHz), and a click every half second."""
    rnd = random.Random(1234)
    frames = []
    n = rate * seconds
    for i in range(n):
        t = i / rate
        sweep = 0.25 * math.sin(2 * math.pi * 20 * seconds / math.log(1000) * (1000 ** (t / seconds) - 1))
        click = 0.5 if i % (rate // 2) == 0 else 0.0
        left = sweep + 0.05 * (rnd.random() * 2 - 1) + click
        right = 0.8 * sweep + 0.05 * (rnd.random() * 2 - 1) - click
        frames.append(struct.pack("<ff", left, right))
    data = b"".join(frames)
    with open(path, "wb") as fh:
        fh.write(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE")
        fh.write(b"fmt " + struct.pack("<IHHIIHH", 16, 3, 2, rate, rate * 8, 8, 32))
        fh.write(b"data" + struct.pack("<I", len(data)) + data)


def main():
    if len(sys.argv) < 4:
        print(__doc__ if __doc__ else "usage: render_matrix.py <ReviewRender> <inputs> <outputs> [--quick]")
        return 2
    exe, inputs, outputs = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
    quick = "--quick" in sys.argv
    os.makedirs(outputs, exist_ok=True)
    # [Claude 2026-10-27] The test signal goes in the temp folder, not the input folder: the input folder is often
    # plugin/sample_audio, which becomes the web demo's track list
    signal = os.path.join(tempfile.gettempdir(), "Test Signal.wav")
    if not os.path.exists(signal):
        write_test_signal(signal)
    failures = 0
    files = [os.path.join(inputs, f) for f in os.listdir(inputs) if f.lower().endswith(".wav") and f != "Test Signal.wav"]
    for path in sorted(files + [signal], key=os.path.basename):
        wav = os.path.basename(path)
        for name, args in SETTINGS:
            if quick and name not in QUICK:
                continue
            out = os.path.join(outputs, os.path.splitext(wav)[0] + "__" + name + ".wav")
            cmd = [exe, "--in", path, "--out", out, "--bits", "32",
                   "--seconds", "8" if quick else "20"] + args
            result = subprocess.run(cmd, capture_output=True, text=True)
            print(("ok   " if result.returncode == 0 else "FAIL ") + os.path.basename(out) + "  " + result.stdout.strip())
            if result.returncode != 0:
                print(result.stderr)
                failures += 1
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
