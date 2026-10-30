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
set(wPLUGIN_CODE                "EXPL")
set(wPLUGIN_CATEGORY            Fx)

# [Claude 2026-10-29] Pipeline and test settings (see README.md, "Tests and the release pipeline")
# Oldest macOS the Mac build runs on; the Mac CI checks it. (No comments after a set() line: the workflows read
# this file line by line.)
set(wMACOS_MIN_VERSION          "11.0")
# The WooCommerce product on the store (Serial Numbers plugin). Required for a v* tag: the pipeline checks the
# product has a working key source before publishing, and sets its "Software version" (what the plugin's
# version check reports) once the new installers are live. Tertiary's is 45.
set(wWOO_PRODUCT_ID             "")

