# The radio parsers and the pure UI helpers live in production .cpp files
# (unlike the older header-only test modules), so this module compiles them
# into a small static library first and links the doctest binary against it.
# The library compiles with the same stdafx.h include the client uses — no
# test-only shims beyond the CRT/Win32 one in stdafx_shim/.
add_library(radio_parsers STATIC
    ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/IcyMetadata.cpp
    ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/RadioStationList.cpp
    ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/TitleTimeline.cpp
    ${CMAKE_SOURCE_DIR}/src/source/UI/Radio/RadioMarquee.cpp
    ${CMAKE_SOURCE_DIR}/src/source/UI/Radio/RadioStatusText.cpp
)
target_include_directories(radio_parsers PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx_shim
    ${CMAKE_SOURCE_DIR}/src/source
    ${CMAKE_SOURCE_DIR}/src/source/App
)
if(MSVC)
    target_compile_options(radio_parsers PRIVATE /utf-8)
endif()

mu_add_test(
    NAME test_radio_parsers
    SOURCES
        test_icy_metadata.cpp
        test_radio_station_list.cpp
        test_radio_title_timeline.cpp
        test_radio_marquee.cpp
        test_radio_status_text.cpp
    LINK_LIBS
        radio_parsers
)

# Streaming measurement harness: runs the production RadioEngine against a
# real station for N minutes and dumps diagnostics CSV (network stalls, push
# gaps, buffer level, underruns). Needs a live stream and minutes of wall
# time, so it is NOT a ctest — build explicitly with -DMU_BUILD_RADIO_HARNESS=ON
# and run by hand:  radio_harness.exe <url> [minutes] [csvPath]
option(MU_BUILD_RADIO_HARNESS "Build the radio streaming measurement harness" OFF)
if(MU_BUILD_RADIO_HARNESS)
    add_executable(radio_harness
        harness/radio_harness.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/RadioEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/IcyMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/TitleTimeline.cpp
    )
    # The shim dir owns stdafx.h / AudioPlayer.h / ErrorReport.h for the
    # engine translation unit: the same seam trick as the stdafx_shim above,
    # plus a real SDL_mixer mixer standing in for AudioPlayer::GetMixer.
    target_include_directories(radio_harness PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/harness/shim
        ${CMAKE_SOURCE_DIR}/src/source
    )
    target_link_libraries(radio_harness PRIVATE
        SDL3::SDL3-static
        SDL3_mixer::SDL3_mixer-static
    )
    if(MSVC)
        target_compile_options(radio_harness PRIVATE /utf-8)
    endif()
endif()
