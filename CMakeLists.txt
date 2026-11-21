cmake_minimum_required(VERSION 3.24)
project(TinykingEXWORD LANGUAGES C ASM)
if(NOT FXSDK_PLATFORM_LONG STREQUAL "exword")
  message(FATAL_ERROR "Use the fxSDK EXWORD.cmake toolchain")
endif()

# Match TheRainbowPhoenix/tinyking-exword's Edelweiss bootstrap procedure.
# Safety test pin: generic syscall-AE scratch execution was removed after
# confirming that the firmware itself overwrites the supposed allocation.
set(EDELWEISS_REF "8ee4e99ba0d99f1e3542e73a5a548062577b8b4a" CACHE STRING "Upstream Edelweiss branch/tag/commit")
set(EXWORD_APP_ID "TKING" CACHE STRING "Five-character installation ID")
set(EXWORD_MODULE "tinyking" CACHE STRING "D01 module name without extension")
set(EXWORD_HEAP_SIZE "262144" CACHE STRING "Payload gint heap")
set(EXWORD_APP_TITLE "TKING" CACHE STRING "EX-word title")
if(NOT EXWORD_APP_ID MATCHES "^[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]$")
  message(FATAL_ERROR "EXWORD_APP_ID must contain exactly 5 uppercase alphanumeric characters")
endif()
string(LENGTH "${EXWORD_MODULE}" MODULE_LENGTH)
if(NOT EXWORD_MODULE MATCHES "^[a-z][a-z0-9_]*$" OR MODULE_LENGTH GREATER 8)
  message(FATAL_ERROR "EXWORD_MODULE must be a lowercase identifier <=8 characters")
endif()
if(NOT EXWORD_HEAP_SIZE MATCHES "^[0-9]+$" OR EXWORD_HEAP_SIZE LESS 256 OR EXWORD_HEAP_SIZE GREATER 786432)
  message(FATAL_ERROR "EXWORD_HEAP_SIZE must be 256..786432")
endif()

include(FetchContent)
set(EDL_BUILD_PAYLOADS OFF CACHE BOOL "Build bundled Edelweiss apps" FORCE)
set(EDL_BUILD_PACKAGES OFF CACHE BOOL "Build bundled Edelweiss packages" FORCE)
set(EDL_APPID "${EXWORD_APP_ID}" CACHE STRING "Embedded bootstrap fallback ID" FORCE)
FetchContent_Declare(edelweiss
  GIT_REPOSITORY https://github.com/TheRainbowPhoenix/Edelweiss.git
  GIT_TAG "${EDELWEISS_REF}")
FetchContent_MakeAvailable(edelweiss)
include("${edelweiss_SOURCE_DIR}/sdk/GenerateEDL.cmake")
include(GenerateEXWORDPackage)

add_gint_edl(tinyking_app
  src/main.c src/core.c src/pak.c src/world.c src/weather.c src/scene.c src/touch.c)
set_target_properties(tinyking_app PROPERTIES OUTPUT_NAME main)
target_compile_definitions(tinyking_app PRIVATE EDL_HEAP_SIZE=${EXWORD_HEAP_SIZE})
target_include_directories(tinyking_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_custom_target(embedded_bootstrap ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_BINARY_DIR}/edelweiss.d01" "${CMAKE_BINARY_DIR}/${EXWORD_MODULE}.d01"
  DEPENDS edelweiss VERBATIM)
if(NOT EXWORD_METADATA_SOURCE)
  message(FATAL_ERROR "Set EXWORD_METADATA_SOURCE to your EX-word metadata repository")
endif()
generate_exword_package(TARGET embedded_bootstrap
  D01 "${CMAKE_BINARY_DIR}/${EXWORD_MODULE}.d01" APPID "${EXWORD_APP_ID}"
  TITLE "${EXWORD_APP_TITLE}" HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_BINARY_DIR}/packages" LOCALES ja cn)

# The game pack stays outside the ELF arena, but is committed to this repo.
# Both EX-word locale packages MUST contain it beside main.edl.
set(TINYKING_PAK "${CMAKE_CURRENT_SOURCE_DIR}/tower.pak"
    CACHE FILEPATH "Path to the GMPK v1 asset pack")
if(NOT EXISTS "${TINYKING_PAK}")
  message(FATAL_ERROR "Missing tower.pak: a TKING package without assets cannot run")
endif()
foreach(LOCALE IN ITEMS ja cn)
  add_custom_target(tinyking_payload_${LOCALE} ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:tinyking_app>"
      "${CMAKE_BINARY_DIR}/packages/${LOCALE}/${EXWORD_APP_ID}/main.edl"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TINYKING_PAK}"
      "${CMAKE_BINARY_DIR}/packages/${LOCALE}/${EXWORD_APP_ID}/tower.pak"
    DEPENDS embedded_bootstrap_package_${LOCALE} tinyking_app VERBATIM)
endforeach()

execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${edelweiss_SOURCE_DIR}"
  OUTPUT_VARIABLE EDELWEISS_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
file(WRITE "${CMAKE_BINARY_DIR}/edelweiss-version.txt" "${EDELWEISS_COMMIT}\n")
message(STATUS "Building with Edelweiss ${EDELWEISS_COMMIT}")
