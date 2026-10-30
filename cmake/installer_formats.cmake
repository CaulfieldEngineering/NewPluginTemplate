# installer_formats.cmake
# [Claude 2026-10-29] Builds the Mac installer's choices from wFORMATS in config.cmake, so a plugin that adds or
# drops AU (or the standalone) gets a matching installer without editing the distribution files.
# Sets, for package_config/mac/distribution-*.xml:
#   WLA_DIST_OUTLINE_FULL / _DEMO    the <line choice=.../> lines
#   WLA_DIST_CHOICES_FULL / _DEMO    the <choice> blocks
#   WLA_DIST_PKGREFS_FULL / _DEMO    the <pkg-ref> lines naming each component package
# The package file names and identifiers match .github/workflows/workflow.yml.

function(wla_dist_part suffix fileSuffix titleSuffix)
    set(outline "")
    set(choices "")
    set(pkgrefs "")
    set(base "com.${wCOMPANY_ID}.${wPLUGIN_NAME}")
    foreach(fmt IN ITEMS VST3 AU Standalone)
        if (NOT fmt IN_LIST wFORMATS)
            continue()
        endif()
        if (fmt STREQUAL "VST3")
            set(id "vst3")
            set(ext "vst3")
            set(what "the VST3 version")
        elseif (fmt STREQUAL "AU")
            set(id "au")
            set(ext "component")
            set(what "the AU (Audio Unit) version, for Logic Pro and GarageBand,")
        else()
            set(id "standalone")
            set(ext "app")
            set(what "the standalone version")
        endif()
        string(APPEND outline "    <line choice=\"${id}\"/>\n")
        string(APPEND choices "  <choice id=\"${id}\" title=\"${wPLUGIN_NAME} - ${fmt}${titleSuffix}\" description=\"Install ${what} of ${wPLUGIN_NAME}\" start_selected=\"true\">\n    <pkg-ref id=\"${base}.${ext}\"/>\n  </choice>\n\n")
        string(APPEND pkgrefs "  <pkg-ref id=\"${base}.${ext}\" version=\"${wPROJECT_VERSION}\">${wPLUGIN_NAME}_${fmt}_${wPROJECT_VERSION}${fileSuffix}.pkg</pkg-ref>\n")
    endforeach()
    set(WLA_DIST_OUTLINE_${suffix} "${outline}" PARENT_SCOPE)
    set(WLA_DIST_CHOICES_${suffix} "${choices}" PARENT_SCOPE)
    set(WLA_DIST_PKGREFS_${suffix} "${pkgrefs}" PARENT_SCOPE)
endfunction()

wla_dist_part(FULL "" "")
wla_dist_part(DEMO ".D" " (Demo)")
