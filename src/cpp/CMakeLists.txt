cmake_minimum_required(VERSION 3.14)

# Turn on IDE target folders in Visual Studio.
#
# This makes each target's `FOLDER` property show up in grouped folders
# instead of one flat list.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Enable modern MSVC runtime handling.
#
# With `NEW`, CMake uses `CMAKE_MSVC_RUNTIME_LIBRARY` / `MSVC_RUNTIME_LIBRARY`
# instead of baking `/MD`, `/MT`, etc. into default flags.
#
# Practical effect: cleaner, explicit control of runtime library selection for
# MSVC builds.
cmake_policy(SET CMP0091 NEW)

################################################################################
# VERSION NUMBER
################################################################################

project("DisplayProber" VERSION "1.1.0" LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)
include(CTest)

################################################################################
# EXE FILE PROPERTIES
################################################################################

# `DP4W_RELEASE_TAG` is passed by GitHub CI from the git release tag.
# `DP4W_VERSION_TAG` is a C++ macro.
set(DP4W_VERSION_TAG "")
if(DEFINED DP4W_RELEASE_TAG AND NOT "${DP4W_RELEASE_TAG}" STREQUAL "")
  set(DP4W_VERSION_TAG "${DP4W_RELEASE_TAG}")
else()
  set(DP4W_VERSION_TAG "v${PROJECT_VERSION}")
endif()

if(NOT DP4W_VERSION_TAG MATCHES "^v")
  set(DP4W_VERSION_TAG "v${DP4W_VERSION_TAG}")
endif()

set(DP4W_VERSION_STRING "${DP4W_VERSION_TAG}")
string(REGEX REPLACE "^v" "" DP4W_VERSION_STRING "${DP4W_VERSION_STRING}")

if(DP4W_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\.([0-9]+))?")
  set(DP4W_VERSION_MAJOR "${CMAKE_MATCH_1}")
  set(DP4W_VERSION_MINOR "${CMAKE_MATCH_2}")
  set(DP4W_VERSION_PATCH "${CMAKE_MATCH_3}")
  if(DEFINED CMAKE_MATCH_5 AND NOT "${CMAKE_MATCH_5}" STREQUAL "")
    set(DP4W_VERSION_BUILD "${CMAKE_MATCH_5}")
  else()
    set(DP4W_VERSION_BUILD "0")
  endif()
else()
  set(DP4W_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
  set(DP4W_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
  set(DP4W_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
  set(DP4W_VERSION_BUILD "0")
endif()

set(DP4W_FILEVERSION_COMMAS
  "${DP4W_VERSION_MAJOR},${DP4W_VERSION_MINOR},${DP4W_VERSION_PATCH},${DP4W_VERSION_BUILD}"
)
set(DP4W_PRODUCTVERSION_COMMAS "${DP4W_FILEVERSION_COMMAS}")

set(DP4W_FILE_VERSION_STR
  "${DP4W_VERSION_MAJOR}.${DP4W_VERSION_MINOR}.${DP4W_VERSION_PATCH}.${DP4W_VERSION_BUILD}"
)
set(DP4W_PRODUCT_VERSION_STR "${DP4W_FILE_VERSION_STR}")

set(DP4W_COMPANY_NAME "Andrew C. Dvorak")
# FileDescription: Human-readable description of this specific file
# (what users see in Properties).
set(DP4W_FILE_DESCRIPTION "CLI to list connected monitors/TVs")
# ProductName: Name of the overall product this file belongs to
# (brand/app/suite name).
set(DP4W_PRODUCT_NAME "${PROJECT_NAME}")
# InternalName: Internal/module identifier for the file, typically the base
# binary name (often without .exe/.dll).
set(DP4W_INTERNAL_NAME "${PROJECT_NAME}")
set(DP4W_ORIGINAL_FILENAME "${PROJECT_NAME}.exe")

string(TIMESTAMP DP4W_COPYRIGHT_YEAR "%Y")
set(DP4W_LEGAL_COPYRIGHT "© ${DP4W_COPYRIGHT_YEAR} Andrew C. Dvorak")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/DisplayProberVersion.rc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/DisplayProberVersion.rc"
  @ONLY
)

################################################################################
# GIT COMMIT AND TIMESTAMP
################################################################################

execute_process(
  COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE DP4W_GIT_COMMIT
  RESULT_VARIABLE DP4W_GIT_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT DP4W_GIT_RESULT EQUAL 0 OR "${DP4W_GIT_COMMIT}" STREQUAL "")
  set(DP4W_GIT_COMMIT "unknown")
endif()

string(TIMESTAMP DP4W_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%S%z")

################################################################################
# BUILD TYPE
################################################################################

set(CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
  CACHE STRING "" FORCE
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release."
    FORCE)
endif()

################################################################################
# SOURCE FILES
################################################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STATIC_CRT "Static runtime" ON)

add_executable(${PROJECT_NAME}
  CLI.cpp
  BasicMonitorInfo.cpp
  DisplayProberInternal.cpp
  DisplayProberLib.cpp
  DxgiOutputDevice.cpp
  EdidBytes.cpp
  GdiDisplayConfig.cpp
  JsonUtils.cpp
  StringUtils.cpp
  SysUtils.cpp
  WmiQueriesInternal.cpp
  WmiQueries.cpp
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE
  dxgi
  mi
  Setupapi
  Shcore
  Wtsapi32
)

set_property(SOURCE CLI.cpp APPEND PROPERTY COMPILE_DEFINITIONS
  DP4W_VERSION_TAG="${DP4W_VERSION_TAG}"
  DP4W_GIT_COMMIT="${DP4W_GIT_COMMIT}"
  DP4W_BUILD_TIMESTAMP="${DP4W_BUILD_TIMESTAMP}"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nemtrif/utfcpp/source
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json/include
)

if(MSVC)
  target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/DisplayProberVersion.rc")

  if(STATIC_CRT)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)

  set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "/DELAYLOAD:api-ms-win-shcore-scaling-l1-1-1.dll /DELAYLOAD:user32.dll /DELAYLOAD:mi.dll /SUBSYSTEM:CONSOLE,6.01 /MANIFEST:EMBED /MANIFESTINPUT:\"${CMAKE_CURRENT_SOURCE_DIR}/DisplayProber.manifest\""
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/Release"
    PDB_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/Debug"
    PDB_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/Release"
  )
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
  # TODO(acdvorak): Remove non-MSVC?
  if(STATIC_CRT)
    target_link_options(${PROJECT_NAME} PRIVATE "-static")
  endif()
endif()

if(BUILD_TESTING)
  add_library(TestStringUtils STATIC
    StringUtils.cpp
  )
  target_compile_features(TestStringUtils PUBLIC cxx_std_20)
  target_include_directories(TestStringUtils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  if(MSVC AND STATIC_CRT)
    set_target_properties(TestStringUtils PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
  endif()

  add_library(TestDisplayProberInternal STATIC
    DisplayProberInternal.cpp
    StringUtils.cpp
  )
  target_compile_features(TestDisplayProberInternal PUBLIC cxx_std_20)
  target_include_directories(TestDisplayProberInternal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json/include
  )
  if(MSVC AND STATIC_CRT)
    set_target_properties(TestDisplayProberInternal PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
  endif()

  add_library(TestWmiQueriesInternal STATIC
    WmiQueriesInternal.cpp
  )
  target_compile_features(TestWmiQueriesInternal PUBLIC cxx_std_20)
  target_include_directories(TestWmiQueriesInternal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  if(MSVC AND STATIC_CRT)
    set_target_properties(TestWmiQueriesInternal PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
  endif()

  add_library(TestJsonUtils STATIC
    JsonUtils.cpp
    tests/stub_SysUtils.cpp
  )
  target_compile_features(TestJsonUtils PUBLIC cxx_std_20)
  target_include_directories(TestJsonUtils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json/include
  )
  if(MSVC AND STATIC_CRT)
    set_target_properties(TestJsonUtils PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
  endif()

  add_library(doctest INTERFACE)
  add_library(doctest::doctest ALIAS doctest)
  target_include_directories(doctest INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/doctest
  )

  add_subdirectory(tests)
endif()
