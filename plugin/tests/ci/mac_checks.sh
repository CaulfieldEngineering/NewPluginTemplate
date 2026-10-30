#!/bin/bash
# mac_checks.sh
# Ported from Tertiary by Claude (2026-10-29). Used by .github/workflows/test.yml on both Macs.
#
#   bash plugin/tests/ci/mac_checks.sh <folder with the built files> <pluginval executable> <name for the results>
#
# Names, codes and formats come from config.cmake (plugin/tests/ci/config.sh).
# 1. Both chip types (arm64, x86_64) in every binary, and the oldest macOS is wMACOS_MIN_VERSION
# 2. The unit tests (<wPLUGIN_NAME>Tests)
# 3. Apple's AU validator (auval) on the AU installed in ~/Library/Audio/Plug-Ins/Components (if wFORMATS has AU)
# 4. pluginval on the VST3 and the AU
# 5. The host test (HostTest) on the VST3 and the AU
# 6. A short offline render (ReviewRender), for the cross-platform comparison
# Logs go in ./results. Exits non-zero on the first failure.

set -euo pipefail
source plugin/tests/ci/config.sh

OUT="$1"
PLUGINVAL="$2"
NAME="$3"
P="$wPLUGIN_NAME"
STRICTNESS="${STRICTNESS:-10}"
MINOS="${wMACOS_MIN_VERSION:-11.0}"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p results

# On any failure, keep macOS crash reports (stack traces) with the logs
trap 'cp ~/Library/Logs/DiagnosticReports/*luginval* ~/Library/Logs/DiagnosticReports/*auval* results/ 2>/dev/null || true' EXIT

echo "=== $NAME: $(sw_vers -productName) $(sw_vers -productVersion), $(uname -m)" | tee results/machine.log

# 1. Architectures and minimum macOS
# ==========================================================================================
check_binary() {
    local bin="$1"
    local archs
    archs=$(lipo -archs "$bin")
    echo "$bin: $archs" | tee -a results/architectures.log
    [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || { echo "ERROR: $bin is not universal"; exit 1; }
    for arch in arm64 x86_64; do
        local minos
        minos=$(vtool -arch "$arch" -show-build "$bin" | awk '/minos/ {print $2}')
        echo "  $arch minimum macOS: $minos" | tee -a results/architectures.log
        [[ "$minos" == "$MINOS" ]] || { echo "ERROR: $bin ($arch) needs macOS $minos, expected $MINOS"; exit 1; }
    done
}
check_binary "$OUT/$P.vst3/Contents/MacOS/$P"
if [ "$HAS_AU" = 1 ]; then check_binary "$OUT/$P.component/Contents/MacOS/$P"; fi
check_binary "$OUT/ReviewRender"
check_binary "$OUT/${P}Tests"
check_binary "$OUT/HostTest"

# 2. Unit tests
# ==========================================================================================
"$OUT/${P}Tests" 2>&1 | tee results/unit-tests.log

# 3. auval, with the codes from config.cmake
# ==========================================================================================
mkdir -p "$AU_DIR" "$VST3_DIR"
rm -rf "$AU_DIR/$P.component" "$VST3_DIR/$P.vst3"
cp -R "$OUT/$P.vst3" "$VST3_DIR/"
if [ "$HAS_AU" = 1 ]; then
    cp -R "$OUT/$P.component" "$AU_DIR/"
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    auval -a 2>&1 | grep -i "$wPLUGIN_MANUFACTURER_CODE" | tee results/auval-list.log || true
    auval -strict -v "$AU_TYPE" "$wPLUGIN_CODE" "$wPLUGIN_MANUFACTURER_CODE" 2>&1 | tee results/auval.log
    grep -q "AU VALIDATION SUCCEEDED" results/auval.log || { echo "ERROR: auval failed"; exit 1; }
else
    echo "SKIP auval: wFORMATS has no AU"
fi

# 4. pluginval, VST3 and AU
# ==========================================================================================
"$PLUGINVAL" --strictness-level "$STRICTNESS" --validate-in-process --output-dir results \
    --validate "$VST3_DIR/$P.vst3" 2>&1 | tee results/pluginval-vst3.log
if [ "$HAS_AU" = 1 ]; then
    "$PLUGINVAL" --strictness-level "$STRICTNESS" --validate-in-process --output-dir results \
        --validate "$AU_DIR/$P.component" 2>&1 | tee results/pluginval-au.log
fi

# 5. The host test: loads the built VST3 and the installed AU as a DAW does (unlicensed runner: demo expected)
# ==========================================================================================
"$OUT/HostTest" --vst3 "$VST3_DIR/$P.vst3" --expect-demo 2>&1 | tee results/hosttest-vst3.log
if [ "$HAS_AU" = 1 ]; then "$OUT/HostTest" --au --expect-demo 2>&1 | tee results/hosttest-au.log; fi

# 6. Render
# ==========================================================================================
python3 plugin/tests/regression/render_matrix.py "$OUT/ReviewRender" plugin/sample_audio results/renders --quick 2>&1 | tee results/renders.log

echo "=== $NAME: all checks passed"
