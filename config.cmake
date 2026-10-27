# config.cmake

# Define global variables
set(wPLUGIN_NAME                "NewPluginTemplate")
set(wCOMPANY_NAME               "Wonderland Audio")
set(wCOMPANY_ID                 "WonderlandAudio")
set(wPROJECT_VERSION            "0.1.0")
set(wPLUGIN_MANUFACTURER_CODE   "WLAD")
set(wFORMATS                    VST3 Standalone)

set(wIS_SYNTH                   FALSE)
set(wNEEDS_MIDI_INPUT           FALSE)
set(wNEEDS_MIDI_OUTPUT          FALSE)
# [Claude 2026-10-27] Plugin code: CHANGE THIS for every new plugin. "Tmpl" is a placeholder;
# the build stops if a renamed plugin still uses it, or uses another Wonderland Audio plugin's code
# (Tertiary is EXPL). The list of taken codes is in cmake/PluginCodes.cmake. The manufacturer
# code above (WLAD) stays the same for every Wonderland Audio plugin.
set(wPLUGIN_CODE                "Tmpl")
set(wPLUGIN_CATEGORY            Fx)