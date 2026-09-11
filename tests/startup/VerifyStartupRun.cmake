execute_process(
  COMMAND "${TEST_EXECUTABLE}" --no-colors
  WORKING_DIRECTORY "${TEST_WORKING_DIR}"
  RESULT_VARIABLE startup_result
  OUTPUT_VARIABLE startup_output
  ERROR_VARIABLE startup_error
  TIMEOUT 15
)
if(NOT startup_result STREQUAL "0"
    OR NOT startup_output MATCHES "assertions: +[1-9][0-9]*"
    OR NOT startup_output MATCHES "Status: SUCCESS!")
  message(FATAL_ERROR
    "Startup cleanup must execute assertions, not just return zero.\n"
    "Result: ${startup_result}\n${startup_output}\n${startup_error}")
endif()
message(STATUS "Startup cleanup ran assertions successfully")
