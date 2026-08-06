# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
#
# LTE core unit tests. These link against the engine shared library (lt) and
# exercise the self-contained core (String, Vector/Array containers, and the
# reflection/type system). They intentionally avoid LTE_Initialize / Window /
# Renderer, so they run headless with no display or audio device.
#
# Build:  python3 configure.py build   (then bin/lte_tests)
# Run:    LD_LIBRARY_PATH=bin:extbin/linux64 bin/lte_tests

set(TEST_SRC
  main.cpp
  Harness.h
  TestString.cpp
  TestVectorArray.cpp
  TestType.cpp
  TestScriptCompile.cpp
  TestShaderAudit.cpp
  TestSFML.cpp
  TestSound.cpp
  TestSerializer.cpp
  TestTexture2D.cpp
  TestWindowInput.cpp
  TestFunctionBind.cpp
  TestStringBindings.cpp
  TestConversionBind.cpp)

add_executable(lte_tests ${TEST_SRC})

# Match the engine's language/standard flags so the LTE headers compile
# cleanly against the same toolchain that built liblt.so.
set_target_properties(lte_tests PROPERTIES CXX_STANDARD 17)
target_compile_options(lte_tests PRIVATE
  -fno-exceptions
  -msse -msse2
  -Wall -Wextra)
target_include_directories(lte_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/liblt
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/FreeType)

# Link the engine shared library (resolved at runtime via LD_LIBRARY_PATH).
target_link_libraries(lte_tests lt ${LINK_LIBRARIES})

configure_output_dir(lte_tests)

# Do not impose -Werror on the test harness itself.

# ---------------------------------------------------------------------------
# ltsl_api_dump: LTSL API database generator. Links liblt, walks the engine
# reflection registries (Function_GetList / Type_GetList) and writes
# script/ltsl-lsp/api-database.json for the LTSL language server.
#   Build:  cmake --build ./build --target ltsl_api_dump
#   Run:    LD_LIBRARY_PATH=bin:extbin/linux64 bin/ltsl_api_dump \
#             script/ltsl-lsp/api-database.json
add_executable(ltsl_api_dump
  ../script/ltsl-lsp/tools/dump_api.cpp)

set_target_properties(ltsl_api_dump PROPERTIES CXX_STANDARD 17)
target_compile_options(ltsl_api_dump PRIVATE
  -fno-exceptions
  -msse -msse2
  -Wall -Wextra)
target_include_directories(ltsl_api_dump PRIVATE
  ${CMAKE_SOURCE_DIR}/src/liblt
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/FreeType)
target_link_libraries(ltsl_api_dump lt ${LINK_LIBRARIES})
configure_output_dir(ltsl_api_dump)
