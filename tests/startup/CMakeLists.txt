if(MSVC)
    configure_file(fixtures/Mix.bmd "${CMAKE_CURRENT_BINARY_DIR}/Data/Local/Mix.bmd" COPYONLY)
    mu_add_test(
        NAME test_startup_diagnostics
        SOURCES test_startup_diagnostics.cpp
            ../../src/source/Core/Utilities/Log/StartupDiagnostics.cpp
    )
    target_include_directories(test_startup_diagnostics BEFORE PRIVATE shim)
    mu_add_test(
        NAME test_character_startup_cleanup
        SOURCES test_character_startup_cleanup.cpp
            ../../src/source/App/Platform/Windows/Winmain.cpp
        LINK_LIBS MuClient
    )
    set_target_properties(test_character_startup_cleanup PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/$<CONFIG>")
    add_dependencies(test_character_startup_cleanup Main)
    add_custom_command(TARGET test_character_startup_cleanup POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DTEST_EXECUTABLE=$<TARGET_FILE:test_character_startup_cleanup>"
            "-DTEST_WORKING_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyStartupRun.cmake"
        VERBATIM)
endif()
