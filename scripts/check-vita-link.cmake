execute_process(COMMAND "${NM}" --undefined-only "${ELF}"
  RESULT_VARIABLE status OUTPUT_VARIABLE symbols ERROR_VARIABLE error)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Cannot inspect Vita executable: ${error}")
endif()
if(symbols MATCHES "[ \t][wUvV][ \t]+(pthread_[A-Za-z0-9_]+|sched_yield)([\r\n]|$)")
  message(FATAL_ERROR "Unresolved threading function: ${CMAKE_MATCH_1}")
endif()
message(STATUS "Vita threading symbols resolved")
