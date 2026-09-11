if(MSVC)
    mu_add_test(
        NAME test_startup_diagnostics
        SOURCES test_startup_diagnostics.cpp
            ../../src/source/Core/Utilities/Log/StartupDiagnostics.cpp
    )
    target_include_directories(test_startup_diagnostics BEFORE PRIVATE shim)
endif()
