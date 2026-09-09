# The radio parsers live in production .cpp files (unlike the older
# header-only test modules), so this module compiles them into a small static
# library first and links the doctest binary against it. The library compiles
# with the same stdafx.h include the client uses — no test-only shims.
add_library(radio_parsers STATIC
    ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/IcyMetadata.cpp
    ${CMAKE_SOURCE_DIR}/src/source/Audio/Radio/RadioStationList.cpp
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
    LINK_LIBS
        radio_parsers
)
