"""Cubase harness generator, driven by config.cmake (Claude, 2026-10-29; generic form of Tertiary's
tertiary_harness_build.py, see ../README.md).

Builds the parts of a Cubase harness project for THIS repo's plugin, in "plugin/tests/daw/cubase/<wPLUGIN_NAME> harness/":

    python make_cubase_harness.py info                     names, VST3 class ID, VST3 bundle path, lane tags
    python make_cubase_harness.py parts [--hosttest EXE | --input WAV]
                                                           Audio/harness-input.wav   HostTest --write-input, tiled to 64 s
                                                           Misc/harness-tempo.smt    the tempo map below (Tempo Track file)
                                                           Misc/harness-track.xml    one audio track with the plugin in
                                                                                     insert 1 and the lanes in
                                                                                     harness_automation.json (Track Archive)
    python make_cubase_harness.py template <cubase-export.xml>
                                                           rewrite track-template.xml from a Cubase File > Export >
                                                           Selected Tracks export (one audio track, any JUCE plugin in
                                                           insert 1): the plugin, its state, its lanes and the paths
                                                           become placeholders

Then assemble_cubase_harness.ps1 imports the parts into a new Cubase project and saves "<name> harness.cpr".

Everything comes from config.cmake: wPLUGIN_NAME, wPLUGIN_MANUFACTURER_CODE, wPLUGIN_CODE. The VST3 class ID of a JUCE
plugin is ABCDEF01 9182FAEB <manufacturer code> <plugin code> (checked 2026-10-29 against Cubase's export of Tertiary),
so no Cubase session is needed to point the track at a different plugin.

The song (4/4, 480 ticks per quarter), bar 1 to bar 13, about 24.8 s: bars 1-4 at 120 BPM, bars 5-6 a ramp to 90
(32 steps), bars 7-8 at 90, bars 9-12 at 140 (a jump). Short on purpose: a WLA plugin without a licence runs a demo
cycle, so a long render may not repeat.

Automation (harness_automation.json): {"lanes": {"<JUCE parameter ID>": [[bar, value], [bar, tick, value], ...]}}
with values 0..1 (normalised) and a step written as two points one tick apart. Cubase's lane tag for a parameter of
the plugin in insert slot 1 is the VST3 parameter ID + 4201; JUCE's VST3 parameter ID is juce::String::hashCode() of
the parameter ID masked to 31 bits.
"""
import glob, json, os, re, struct, subprocess, sys, tempfile
from xml.sax.saxutils import escape

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
TEMPLATE = os.path.join(HERE, "track-template.xml")
AUTOMATION_FILE = os.path.join(HERE, "harness_automation.json")

PPQ = 480
BAR = 4 * PPQ
SONG_END_BAR = 13
LANE_OFFSET = 4201
JUCE_VST3_PREFIX = "ABCDEF019182FAEB"


# ---- config.cmake ---------------------------------------------------------------------------------------------------

def config():
    text = open(os.path.join(REPO, "config.cmake"), encoding="utf-8").read()
    vals = dict(re.findall(r'set\(\s*(w[A-Z_]+)\s+"?([^")\s]*)"?', text))
    for key in ("wPLUGIN_NAME", "wPLUGIN_MANUFACTURER_CODE", "wPLUGIN_CODE"):
        if not vals.get(key):
            raise SystemExit("config.cmake has no %s" % key)
    return vals


def plugin_guid(cfg):
    four = lambda code: "".join("%02X" % ord(c) for c in code)
    return JUCE_VST3_PREFIX + four(cfg["wPLUGIN_MANUFACTURER_CODE"]) + four(cfg["wPLUGIN_CODE"])


def harness_dir(cfg):
    return os.path.join(HERE, cfg["wPLUGIN_NAME"] + " harness")


def vst3_path(cfg):
    name = cfg["wPLUGIN_NAME"]
    for b in ("build", "build-tests"):
        p = os.path.join(REPO, b, "plugin", name + "_artefacts", "Release", "VST3", name + ".vst3")
        if os.path.isdir(p):
            return p
    return os.path.join(REPO, "build", "plugin", name + "_artefacts", "Release", "VST3", name + ".vst3") + "  (not built yet)"


def juce_hash(s):
    """juce::String::hashCode (31 * h + char, 32-bit), masked to 31 bits as JUCE's VST3 wrapper does."""
    h = 0
    for c in s:
        h = (31 * h + ord(c)) & 0xFFFFFFFF
    return h & 0x7FFFFFFF


# ---- tempo ----------------------------------------------------------------------------------------------------------

def bar(b, extra=0):
    return (b - 1) * BAR + extra


def tempo_events():
    ev = [(0, 120.0)]
    start, end = bar(5), bar(7)
    steps = (end - start) // (PPQ // 4)
    for k in range(steps):
        ev.append((start + k * (PPQ // 4), 120.0 + (90.0 - 120.0) * k / steps))
    ev.append((bar(7), 90.0))
    ev.append((bar(9), 140.0))
    return ev


def seconds_at(tick):
    ev = tempo_events() + [(10 ** 9, 0)]
    s = 0.0
    for (t0, bpm), (t1, _) in zip(ev, ev[1:]):
        if tick <= t0:
            break
        s += (min(tick, t1) - t0) / PPQ * 60.0 / bpm
    return s


def write_smt(path):
    ids = iter(range(710000000, 720000000))
    nid = lambda: next(ids)
    evs = "\n".join('         <obj class="MTempoEvent" ID="%d">\n            <float name="BPM" value="%.6f"/>\n'
                    '            <float name="PPQ" value="%d"/>\n         </obj>' % (nid(), bpm, t)
                    for t, bpm in tempo_events())
    tempo_id, sig_id, tvc1, tv1, tvc2, tv2, sig_ev = (nid() for _ in range(7))

    def attrs(tlid, tvc, tv):
        return ('      <member name="Additional Attributes">\n'
                '         <int name="TLID" value="%d"/>\n         <int name="LocS" value="0"/>\n'
                '         <int name="Farb" value="5"/>\n'
                '         <obj class="MTrackVariationCollection" name="TVCi" ID="%d">\n'
                '            <int name="ownership" value="2"/>\n            <list name="obj" type="obj">\n'
                '               <obj class="MTrackVariation" ID="%d">\n'
                '                  <string name="name" value="v1" wide="true"/>\n'
                '                  <int name="variationID" value="0"/>\n               </obj>\n'
                '            </list>\n         </obj>\n         <int name="Lock" value="0"/>\n'
                '         <int name="Eths" value="-260498236"/>\n      </member>') % (tlid, tvc, tv)

    xml = ('<?xml version="1.0" encoding="utf-8"?>\n<MasterTrack>\n   <rootObjects>\n'
           '      <root name="Tempo Track" ID="%d"/>\n      <root name="Signature Track" ID="%d"/>\n'
           '   </rootObjects>\n\n   <obj class="MTempoTrackEvent" ID="%d">\n      <list name="TempoEvent" type="obj">\n'
           '%s\n      </list>\n%s\n      <int name="Flags" value="1"/>\n   </obj>\n\n'
           '   <obj class="MSignatureTrackEvent" ID="%d">\n      <list name="SignatureEvent" type="obj">\n'
           '         <obj class="MTimeSignatureEvent" ID="%d">\n            <int name="Flags" value="8"/>\n'
           '            <float name="Start" value="0"/>\n            <float name="Length" value="1"/>\n'
           '            <int name="Bar" value="0"/>\n            <int name="Numerator" value="4"/>\n'
           '            <int name="Denominator" value="4"/>\n            <int name="Position" value="0"/>\n'
           '         </obj>\n      </list>\n%s\n      <int name="Flags" value="1"/>\n   </obj>\n</MasterTrack>\n'
           ) % (tempo_id, sig_id, tempo_id, evs, attrs(1, tvc1, tv1), sig_id, sig_ev, attrs(2, tvc2, tv2))
    open(path, "w", encoding="utf-8").write(xml)
    print("%s: %d tempo events; song end (bar %d) at %.3f s" % (path, len(tempo_events()), SONG_END_BAR,
                                                                 seconds_at(bar(SONG_END_BAR))))


# ---- input audio ----------------------------------------------------------------------------------------------------

def tile_input(src, dst, seconds=64):
    d = open(src, "rb").read()
    i, fmt, data = 12, None, None
    while i + 8 <= len(d):
        cid, size = d[i:i + 4], struct.unpack("<I", d[i + 4:i + 8])[0]
        if cid == b"fmt ":
            fmt = d[i + 8:i + 8 + size]
        elif cid == b"data":
            data = d[i + 8:i + 8 + size]
        i += 8 + size + (size & 1)
    tag, ch, rate, _, align, bits = struct.unpack("<HHIIHH", fmt[:16])
    frames = int(seconds * rate)
    reps = -(-frames * align // len(data))
    body = (data * reps)[:frames * align]
    out = b"fmt " + struct.pack("<I", 16) + fmt[:16] + b"data" + struct.pack("<I", len(body)) + body
    open(dst, "wb").write(b"RIFF" + struct.pack("<I", 4 + len(out)) + b"WAVE" + out)
    print("%s: %d s, %d Hz, %d ch, %d-bit, the input repeated" % (dst, seconds, rate, ch, bits))


def find_hosttest():
    exe = "HostTest.exe" if os.name == "nt" else "HostTest"
    for b in ("build-tests", "build"):
        for c in glob.glob(os.path.join(REPO, b, "plugin", "HostTest_artefacts", "*", exe)):
            return c
    raise SystemExit("no HostTest build (plugin/tests/run_all.ps1 builds it in build-tests/); "
                     "pass --hosttest <exe> or --input <wav>")


def write_input(dst, hosttest=None, input_wav=None):
    if input_wav:
        return tile_input(input_wav, dst)
    hosttest = hosttest or find_hosttest()
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "input-2s.wav")
        subprocess.run([hosttest, "--write-input", src], check=True, cwd=tmp)
        tile_input(src, dst)


# ---- track archive --------------------------------------------------------------------------------------------------

LANE = '''               <obj class="MAutomationTrackEvent" ID="{id1}">
                  <float name="Start" value="0"/>
                  <float name="Length" value="658299.432"/>
                  <obj class="MAutoListNode" name="Node" ID="{id2}">
                     <member name="Domain">
                        <int name="Type" value="0"/>
                        <obj name="Tempo Track" ID="{tempo}"/>
                        <obj name="Signature Track" ID="{sig}"/>
                     </member>
                     <list name="Events" type="obj">
{events}
                     </list>
                  </obj>
                  <member name="Additional Attributes">
                     <int name="Eths" value="-260498236"/>
                  </member>
{device}
                  <int name="Tag" value="{tag}"/>
               </obj>'''

# The first lane of a device defines it; Cubase's later lanes for the same device refer to it by ID.
DEVICE = '''                  <obj class="MAutomationTrack" name="Track Device" ID="{id}">
                     <int name="Connection Type" value="7"/>
                     <string name="Device Name" value="Inserts\\Slot\\{guid}-0"/>
                     <int name="Read" value="1"/>
                     <int name="Write" value="0"/>
                  </obj>'''
DEVICE_REF = '                  <obj name="Track Device" ID="{id}"/>'

EVENT = ('                        <obj class="MParamEvent" ID="{id}">\n'
         '                           <float name="Start" value="{tick}"/>\n'
         '                           <float name="Value" value="{value}"/>\n'
         '                        </obj>')


def load_automation():
    if not os.path.exists(AUTOMATION_FILE):
        return {}
    lanes = json.load(open(AUTOMATION_FILE, encoding="utf-8")).get("lanes", {})
    out = {}
    for pid, pts in lanes.items():
        out[pid] = [(bar(p[0]), p[1]) if len(p) == 2 else (bar(p[0], p[1]), p[2]) for p in pts]
    return out


def write_archive(cfg, out):
    t = open(TEMPLATE, encoding="utf-8").read()
    guid = plugin_guid(cfg)
    hdir = harness_dir(cfg)
    m = re.search(r'<member name="Domain">\s*<int name="Type" value="0"/>\s*<obj name="Tempo Track" ID="(\d+)"/>\s*'
                  r'<obj name="Signature Track" ID="(\d+)"/>', t)
    ids = iter(range(1900000000, 1999999999))
    lanes = []
    dev = next(ids)
    for pid, points in load_automation().items():
        events = "\n".join(EVENT.format(id=next(ids), tick=tick, value=value) for tick, value in points)
        device = (DEVICE if not lanes else DEVICE_REF).format(id=dev, guid=guid)
        lanes.append(LANE.format(id1=next(ids), id2=next(ids), tempo=m.group(1), sig=m.group(2),
                                 events=events, device=device, tag=juce_hash(pid) + LANE_OFFSET))
    t = t.replace("<!--PLUGIN_LANES-->", "\n".join(lanes))
    t = t.replace("{{PLUGIN_GUID}}", guid).replace("{{PLUGIN_NAME}}", escape(cfg["wPLUGIN_NAME"], {'"': "&quot;"}))
    t = t.replace("{{HARNESS_DIR}}", escape(hdir, {'"': "&quot;"}))
    open(out, "w", encoding="utf-8").write(t)
    print("%s: %s (%s) in insert 1, %d automation lane(s)" % (out, cfg["wPLUGIN_NAME"], guid, len(lanes)))


def derive_template(export_path):
    """A Cubase Selected Tracks export -> track-template.xml with placeholders. Keeps everything Cubase wrote except
    the plugin's identity, its saved state (so it opens at its defaults) and its parameter lanes."""
    t = open(export_path, encoding="utf-8").read()
    slot = t.index('<member name="InsertFolder">')
    m = re.compile(r'<string name="GUID" value="([0-9A-F]{32})" wide="true"/>\s*</member>\s*'
                   r'<string name="Plugin Name" value="([^"]*)"').search(t, slot)
    guid, name = m.group(1), m.group(2)
    if not guid.startswith(JUCE_VST3_PREFIX):
        print("warning: insert 1 is %s (%s), not a JUCE plugin" % (name, guid))
    t = t[:m.start()] + m.group(0).replace(guid, "{{PLUGIN_GUID}}").replace('value="%s"' % name,
                                                                            'value="{{PLUGIN_NAME}}"') + t[m.end():]
    b = re.compile(r'(<bin name="audioComponent">).*?(</bin>)', re.S).search(t, m.start())
    t = t[:b.start()] + b.group(1) + "\n                           " + b.group(2) + t[b.end():]
    t = t.replace(guid, "{{PLUGIN_GUID}}")
    # the plugin's lanes go; the first one's place becomes the marker the generator fills
    marker = '<obj class="MAutomationTrackEvent"'
    parts = t.split(marker)
    kept, placed, dev = [parts[0]], False, None
    for body in parts[1:]:
        end = body.index("<int name=\"Tag\"")
        end = body.index("</obj>", end) + len("</obj>")
        lane = body[:end]
        d = re.search(r'<obj class="MAutomationTrack" name="Track Device" ID="(\d+)">\s*<int name="Connection Type" '
                      r'value="\d+"/>\s*<string name="Device Name" value="Inserts\\Slot\\\{\{PLUGIN_GUID\}\}', lane)
        if d:
            dev = d.group(1)
        if d or (dev and '<obj name="Track Device" ID="%s"/>' % dev in lane):
            tail = body[end:]
            if not placed:
                kept[-1] = kept[-1].rstrip(" ") + "<!--PLUGIN_LANES-->"
                placed = True
            kept[-1] += tail
        else:
            kept.append(body)
    t = marker.join(kept)
    if not placed:
        t = t.replace('<list name="Tracks" type="obj">', '<list name="Tracks" type="obj">\n<!--PLUGIN_LANES-->', 1)
    # media paths: the export's harness folder becomes a placeholder
    t = re.sub(r'value="[^"]*\\(Audio|Misc)\\"', lambda mm: 'value="{{HARNESS_DIR}}\\%s\\"' % mm.group(1), t)
    open(TEMPLATE, "w", encoding="utf-8").write(t)
    print("%s: from %s (insert 1 was %s, %s)" % (TEMPLATE, export_path, name, guid))


# ---- main -----------------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    args = sys.argv[1:]
    cmd = args[0] if args else ""
    opt = lambda k: args[args.index(k) + 1] if k in args else None
    if cmd == "template":
        derive_template(args[1])
    elif cmd in ("parts", "info"):
        cfg = config()
        h = harness_dir(cfg)
        if cmd == "info":
            print("plugin       %s" % cfg["wPLUGIN_NAME"])
            print("VST3 class   %s" % plugin_guid(cfg))
            print("VST3 bundle  %s" % vst3_path(cfg))
            print("harness      %s" % os.path.join(h, cfg["wPLUGIN_NAME"] + " harness.cpr"))
            for pid in load_automation():
                print("lane         %-24s VST3 id %11d  Cubase tag %11d" % (pid, juce_hash(pid), juce_hash(pid) + LANE_OFFSET))
            sys.exit(0)
        for sub in ("Audio", "Misc"):
            os.makedirs(os.path.join(h, sub), exist_ok=True)
        write_input(os.path.join(h, "Audio", "harness-input.wav"), opt("--hosttest"), opt("--input"))
        write_smt(os.path.join(h, "Misc", "harness-tempo.smt"))
        write_archive(cfg, os.path.join(h, "Misc", "harness-track.xml"))
    else:
        print(__doc__)
        sys.exit(2)
