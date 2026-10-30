#!/bin/bash
# config.sh: reads config.cmake into shell variables, for the CI scripts in this folder.
# Written by Claude (2026-10-29) for the template. Source it from the repository root:
#     source plugin/tests/ci/config.sh
# Sets every w* value in config.cmake (wPLUGIN_NAME, wCOMPANY_ID, wPROJECT_VERSION, wFORMATS, ...), and:
#     AU_TYPE        aufx (effect) or aumu (synth), from wIS_SYNTH, for auval
#     HAS_AU, HAS_VST3, HAS_STANDALONE   1 or 0, from wFORMATS
# The same parsing as the workflows: one set(wKEY value) per line, no comment after it.

CONFIG_FILE="${CONFIG_FILE:-config.cmake}"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "config.sh: $CONFIG_FILE not found (run from the repository root)" >&2
    return 1 2>/dev/null || exit 1
fi
while IFS=' ' read -r key value; do
    value=$(echo $value | tr -d '"')
    export "$key=$value"
done < <(grep 'set(w' "$CONFIG_FILE" | sed -e 's/set(//g' -e 's/)//g' -e 's/\s\+/ /g' | tr -d '\r')

case "$(echo "${wIS_SYNTH:-FALSE}" | tr '[:lower:]' '[:upper:]')" in
    TRUE|ON|YES|1) export AU_TYPE=aumu ;;
    *)             export AU_TYPE=aufx ;;
esac
wla_has_format() { case " $wFORMATS " in *" $1 "*) echo 1 ;; *) echo 0 ;; esac; }
HAS_AU=$(wla_has_format AU)
HAS_VST3=$(wla_has_format VST3)
HAS_STANDALONE=$(wla_has_format Standalone)
export HAS_AU HAS_VST3 HAS_STANDALONE
unset -f wla_has_format
