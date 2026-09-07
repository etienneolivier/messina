cmake_minimum_required(VERSION 3.22)

project(Messina VERSION 0.0.1 LANGUAGES C CXX)

# Point to local JUCE (downloaded to GitHub/Libs/JUCE)
add_subdirectory(../../Libs/JUCE JUCE)

juce_add_plugin(Messina
    COMPANY_NAME "malcolm audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Osme"
    PLUGIN_CODE "Mess"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Messina"
    BUNDLE_ID "com.osme.messina"
)

juce_generate_juce_header(Messina)

target_sources(Messina
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/MessinaDSP.h
)

target_compile_features(Messina PRIVATE cxx_std_20)

# Assets
juce_add_binary_data(MessinaAssets
    SOURCES
        Source/ui.html
)

target_link_libraries(Messina
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_graphics
        MessinaAssets
)

target_compile_definitions(Messina
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# Copy ui.html to plugin bundle Resources (macOS only)
# Windows VST3 bundles don't need this - ui.html is embedded via binary data
if(APPLE)
    add_custom_command(TARGET Messina_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Messina_VST3>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui.html"
            "$<TARGET_BUNDLE_DIR:Messina_VST3>/Contents/Resources/ui.html"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Messina.vst3/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui.html"
            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Messina.vst3/Contents/Resources/ui.html"
        COMMENT "Copying ui.html to VST3 bundle + installed location (macOS)"
    )

    add_custom_command(TARGET Messina_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Messina_AU>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui.html"
            "$<TARGET_BUNDLE_DIR:Messina_AU>/Contents/Resources/ui.html"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components/Messina.component/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui.html"
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components/Messina.component/Contents/Resources/ui.html"
        COMMENT "Copying ui.html to AU bundle + installed location (macOS)"
    )
endif()
