add_executable(cpp_tests
  doctest_main.cpp
  test_StringUtils.cpp
)

target_compile_features(cpp_tests PRIVATE cxx_std_20)
target_link_libraries(cpp_tests PRIVATE
  TestStringUtils
  doctest::doctest
)

if(MSVC AND STATIC_CRT)
  set_target_properties(cpp_tests PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
endif()

if(MSVC)
  set(DP4W_TEST_OUTPUT_ROOT "${CMAKE_BINARY_DIR}/tests")

  set_target_properties(cpp_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${DP4W_TEST_OUTPUT_ROOT}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DP4W_TEST_OUTPUT_ROOT}/Release"
    PDB_OUTPUT_DIRECTORY_DEBUG "${DP4W_TEST_OUTPUT_ROOT}/Debug"
    PDB_OUTPUT_DIRECTORY_RELEASE "${DP4W_TEST_OUTPUT_ROOT}/Release"
  )
endif()

set(DP4W_DOCTEST_CMAKE_HELPER "")

if(TARGET doctest::doctest)
  get_target_property(DP4W_DOCTEST_INCLUDE_DIRS doctest::doctest INTERFACE_INCLUDE_DIRECTORIES)
  foreach(DP4W_DOCTEST_INCLUDE_DIR IN LISTS DP4W_DOCTEST_INCLUDE_DIRS)
    if(EXISTS "${DP4W_DOCTEST_INCLUDE_DIR}/scripts/cmake/doctest.cmake")
      set(DP4W_DOCTEST_CMAKE_HELPER "${DP4W_DOCTEST_INCLUDE_DIR}/scripts/cmake/doctest.cmake")
      break()
    endif()
  endforeach()
endif()

if(DP4W_DOCTEST_CMAKE_HELPER STREQUAL "")
  set(DP4W_DOCTEST_FALLBACK_HELPER
    "${CMAKE_CURRENT_LIST_DIR}/../third_party/doctest/scripts/cmake/doctest.cmake"
  )
  if(EXISTS "${DP4W_DOCTEST_FALLBACK_HELPER}")
    set(DP4W_DOCTEST_CMAKE_HELPER "${DP4W_DOCTEST_FALLBACK_HELPER}")
  endif()
endif()

if(DP4W_DOCTEST_CMAKE_HELPER STREQUAL "")
  message(FATAL_ERROR "doctest.cmake not found; cannot register per-TEST_CASE CTest entries")
endif()

include("${DP4W_DOCTEST_CMAKE_HELPER}")
doctest_discover_tests(cpp_tests
  DISCOVERY_MODE PRE_TEST
)
