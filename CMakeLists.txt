cmake_minimum_required(VERSION 3.24)
project(ExwordEdelweissExample LANGUAGES C ASM)
if(NOT FXSDK_PLATFORM_LONG STREQUAL "exword")
  message(FATAL_ERROR "Use the fxSDK EXWORD.cmake toolchain")
endif()

# Keep this branch to follow the latest EX-word runtime, or pin a commit/tag.
set(EDELWEISS_REF "edelweiss" CACHE STRING "Upstream Edelweiss branch, tag or commit")
set(EXWORD_APP_ID "EXMPL" CACHE STRING "Five-character dictionary installation ID")
set(EXWORD_MODULE "example" CACHE STRING "D01 filename without its extension")
set(EXWORD_HEAP_SIZE "262144" CACHE STRING "Payload-local gint heap in bytes")
set(EXWORD_APP_TITLE "Edelweiss ELF example" CACHE STRING "Installation title")
if(NOT EXWORD_APP_ID MATCHES "^[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]$")
  message(FATAL_ERROR "EXWORD_APP_ID must be five uppercase letters/digits")
endif()
string(LENGTH "${EXWORD_MODULE}" MODULE_LENGTH)
if(NOT EXWORD_MODULE MATCHES "^[a-z][a-z0-9_]*$" OR MODULE_LENGTH GREATER 8)
  message(FATAL_ERROR "EXWORD_MODULE must be a lowercase identifier of at most eight characters")
endif()

include(FetchContent)
set(EDL_BUILD_PAYLOADS OFF CACHE BOOL "Build upstream bundled payloads" FORCE)
set(EDL_BUILD_PACKAGES OFF CACHE BOOL "Build upstream installation packages" FORCE)
set(EDL_APPID "${EXWORD_APP_ID}" CACHE STRING "Embedded bootstrap fallback ID" FORCE)
FetchContent_Declare(edelweiss
  GIT_REPOSITORY https://github.com/TheRainbowPhoenix/Edelweiss.git
  GIT_TAG "${EDELWEISS_REF}")
FetchContent_MakeAvailable(edelweiss)
include("${edelweiss_SOURCE_DIR}/sdk/GenerateEDL.cmake")
include(GenerateEXWORDPackage)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Reuse the upstream example's wallpaper generator. The RGB565 wallpaper alone
# occupies 168,960 loaded bytes, demonstrating payloads above the D01 limit.
set(ASSETS "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command(OUTPUT "${ASSETS}/ebrew_assets.h" "${ASSETS}/wallpaper.c"
  "${ASSETS}/splash.c"
  COMMAND "${Python3_EXECUTABLE}" "${edelweiss_SOURCE_DIR}/tools/generate_assets.py" "${ASSETS}"
  DEPENDS "${edelweiss_SOURCE_DIR}/tools/generate_assets.py"
    "${edelweiss_SOURCE_DIR}/assets/CASIO_EX.bmp" VERBATIM)
add_custom_target(example_assets DEPENDS "${ASSETS}/ebrew_assets.h" "${ASSETS}/wallpaper.c")
add_gint_edl(example_app src/main.c "${ASSETS}/wallpaper.c")
set_target_properties(example_app PROPERTIES OUTPUT_NAME main)
if(NOT EXWORD_HEAP_SIZE MATCHES "^[0-9]+$" OR EXWORD_HEAP_SIZE LESS 256 OR EXWORD_HEAP_SIZE GREATER 786432)
  message(FATAL_ERROR "EXWORD_HEAP_SIZE must be 256..786432 bytes; code/assets also occupy the ELF arena")
endif()
target_compile_definitions(example_app PRIVATE EDL_HEAP_SIZE=${EXWORD_HEAP_SIZE})
target_include_directories(example_app PRIVATE "${ASSETS}")
add_dependencies(example_app example_assets)

add_custom_target(embedded_bootstrap ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_BINARY_DIR}/edelweiss.d01"
    "${CMAKE_BINARY_DIR}/${EXWORD_MODULE}.d01"
  DEPENDS edelweiss VERBATIM)
if(NOT EXWORD_METADATA_SOURCE)
  message(FATAL_ERROR "Set EXWORD_METADATA_SOURCE to the EX-word metadata repository (CI does this)")
endif()
generate_exword_package(TARGET embedded_bootstrap
  D01 "${CMAKE_BINARY_DIR}/${EXWORD_MODULE}.d01" APPID "${EXWORD_APP_ID}"
  TITLE "${EXWORD_APP_TITLE}" HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_BINARY_DIR}/packages" LOCALES ja cn)
foreach(LOCALE IN ITEMS ja cn)
  add_custom_target(example_payload_${LOCALE} ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:example_app>"
      "${CMAKE_BINARY_DIR}/packages/${LOCALE}/${EXWORD_APP_ID}/main.edl"
    DEPENDS embedded_bootstrap_package_${LOCALE} example_app VERBATIM)
endforeach()
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${edelweiss_SOURCE_DIR}"
  OUTPUT_VARIABLE EDELWEISS_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
file(WRITE "${CMAKE_BINARY_DIR}/edelweiss-version.txt" "${EDELWEISS_COMMIT}\n")
message(STATUS "Building with Edelweiss ${EDELWEISS_COMMIT}")
