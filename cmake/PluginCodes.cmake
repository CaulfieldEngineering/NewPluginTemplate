# PluginCodes.cmake
# Written by Claude (2026-10-27), approved by Joe ("NewPluginTemplate could be fixed too").
#
# Stops a new plugin from shipping with the template's plugin code, or with a code another
# Wonderland Audio plugin already uses. Hosts identify a plugin by its manufacturer code plus its
# plugin code (the AU type/subtype, and JUCE builds the VST3 ID from the same two). Two plugins with
# the same pair look like the same plugin: one replaces the other in Logic, Cubase, Reaper...
#
# The manufacturer code (WLAD) is shared on purpose: it means "Wonderland Audio" and groups all our
# plugins under one maker in every DAW. So the plugin code is what must be unique.
#
# Included from CMakeLists.txt right after config.cmake.

# The template's placeholder. Mixed case on purpose: an AU plugin code needs exactly one capital
# letter, so a prototype still builds and loads before it gets its real code.
set(TEMPLATE_PLUGIN_CODE "Tmpl")

# Plugin codes already used by Wonderland Audio plugins (name:code). Add yours when you pick it.
set(WLA_TAKEN_PLUGIN_CODES
    "Tertiary:EXPL"     # Shipped. Never reuse.
    "Brownout:BRWN"
    "Rolloff:RLOF"
    "Fattener:FATN"
    "Tightrope:TGHT"
)

if (wPLUGIN_CODE STREQUAL TEMPLATE_PLUGIN_CODE)
    if (wPLUGIN_NAME STREQUAL "NewPluginTemplate")
        message(WARNING "PluginCodes.cmake: building the template itself with the placeholder plugin code '${TEMPLATE_PLUGIN_CODE}'. A real plugin must set its own wPLUGIN_CODE in config.cmake.")
    else()
        message(FATAL_ERROR "PluginCodes.cmake: '${wPLUGIN_NAME}' still uses the template's plugin code '${TEMPLATE_PLUGIN_CODE}'. "
                            "Set a unique 4-character wPLUGIN_CODE in config.cmake (one capital letter, e.g. 'Brwn'), "
                            "and add it to WLA_TAKEN_PLUGIN_CODES in cmake/PluginCodes.cmake.")
    endif()
endif()

foreach (entry IN LISTS WLA_TAKEN_PLUGIN_CODES)
    string(REPLACE ":" ";" pair "${entry}")
    list(GET pair 0 owner)
    list(GET pair 1 code)
    if (wPLUGIN_CODE STREQUAL code AND NOT wPLUGIN_NAME STREQUAL owner)
        message(FATAL_ERROR "PluginCodes.cmake: plugin code '${code}' belongs to ${owner}. "
                            "'${wPLUGIN_NAME}' would replace it in every DAW. Pick another wPLUGIN_CODE in config.cmake.")
    endif()
endforeach()

if (NOT wPLUGIN_MANUFACTURER_CODE STREQUAL "WLAD")
    message(WARNING "PluginCodes.cmake: manufacturer code is '${wPLUGIN_MANUFACTURER_CODE}', not Wonderland Audio's 'WLAD'. "
                    "DAWs will list this plugin under a different maker. Fine for an experiment, not for a product.")
endif()
