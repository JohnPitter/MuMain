foreach(required MU_COMPILER MU_PREFIX MU_PROBE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing MSVC dependency probe input: ${required}")
  endif()
endforeach()

file(MAKE_DIRECTORY "${MU_PROBE_DIR}")
file(WRITE "${MU_PROBE_DIR}/dependency_probe.h" "#pragma once\n")
file(WRITE "${MU_PROBE_DIR}/dependency_probe.cpp" "#include \"dependency_probe.h\"\n")
execute_process(
  COMMAND "${MU_COMPILER}" /nologo /showIncludes /c dependency_probe.cpp
  WORKING_DIRECTORY "${MU_PROBE_DIR}"
  OUTPUT_VARIABLE probe_output
  ERROR_VARIABLE probe_error
  RESULT_VARIABLE probe_result
  ENCODING UTF-8
)
string(FIND "${probe_output}" "${MU_PREFIX}" prefix_position)
if(NOT probe_result EQUAL 0 OR prefix_position LESS 0)
  message(FATAL_ERROR
    "MSVC include output does not match Ninja's dependency prefix. "
    "The build is unsafe: changed headers may reuse incompatible objects. "
    "Use tools/cmake-windows.cmd for configure AND build. "
    "Reconfigure into a NEW build directory when recovering an existing cache. "
    "Compiler status: ${probe_result}. ${probe_error}")
endif()
