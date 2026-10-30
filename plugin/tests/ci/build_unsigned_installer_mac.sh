#!/bin/bash
# build_unsigned_installer_mac.sh
# Ported from Tertiary by Claude (2026-10-29). Self-test only (installer-test.yml on a claude/installer-test branch):
# builds the Mac installer the same way workflow.yml does (component packages, then productbuild with
# distribution-full.xml), but UNSIGNED, so the install and uninstall checks can be tried before anything goes to
# staging. The real test uses the signed, notarized installer from the staging pipeline.
#
#   bash plugin/tests/ci/build_unsigned_installer_mac.sh <cmake build folder> <output folder>

set -euo pipefail
source plugin/tests/ci/config.sh
BUILD="$1"
OUT="$2"
NAME="$wPLUGIN_NAME"
VERSION="$wPROJECT_VERSION"
ID="com.$wCOMPANY_ID.$NAME"
ART="$BUILD/plugin/${NAME}_artefacts/Release"
mkdir -p "$OUT" "$BUILD/output"

# Not relocatable, as in workflow.yml (else the installer may update another copy it knows of, e.g. in build/).
# Same identifiers, package names and install locations as workflow.yml.
component() {   # $1 artefact folder and format name, $2 identifier suffix, $3 install location
    pkgbuild --analyze --root "$ART/$1" "$BUILD/output/$2.plist" >/dev/null
    plutil -replace BundleIsRelocatable -bool NO "$BUILD/output/$2.plist"
    pkgbuild --component-plist "$BUILD/output/$2.plist" --root "$ART/$1" --identifier "$ID.$2" --version "$VERSION" \
             --install-location "$3" "$BUILD/output/${NAME}_$1_${VERSION}.pkg"
}
if [ "$HAS_STANDALONE" = 1 ]; then component Standalone app "/Applications/$NAME"; fi
if [ "$HAS_VST3" = 1 ]; then component VST3 vst3 "/Library/Audio/Plug-Ins/VST3"; fi
if [ "$HAS_AU" = 1 ]; then component AU component "/Library/Audio/Plug-Ins/Components"; fi

cp "$BUILD/distribution-full.xml" "$BUILD/output/distribution.xml"
productbuild --distribution "$BUILD/output/distribution.xml" --package-path "$BUILD/output" \
             "$OUT/${NAME}MacInstaller_${VERSION}.pkg"
ls -la "$OUT"
