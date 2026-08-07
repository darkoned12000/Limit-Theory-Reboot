# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

cmake_minimum_required (VERSION 3.10...4.2)
project(LTheory)

# Prefer GLVND OpenGL libraries on modern systems (silences CMP0072)
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

find_package(OpenGL REQUIRED)

# SFML 3.1: find the system-installed package
find_package(SFML 3.1 REQUIRED COMPONENTS Graphics Audio Network System Window)

function (configure_output_dir target)
  set_target_properties (${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

  foreach (config ${CMAKE_CONFIGURATION_TYPES})
    string (TOUPPER ${config} config)
    set_target_properties (${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_SOURCE_DIR}/bin"
      LIBRARY_OUTPUT_DIRECTORY_${config} "${CMAKE_SOURCE_DIR}/bin"
      ARCHIVE_OUTPUT_DIRECTORY_${config} "${CMAKE_SOURCE_DIR}/bin")
  endforeach (config)
endfunction ()

# Set build path
set(BUILD_DIR ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${BUILD_DIR} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${BUILD_DIR} CACHE PATH "Build directory" FORCE)

# Detect OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MAC 1)
else()
  message(FATAL_ERROR "Unsupported operating system")
  return()
endif()

# Detect Architecture
include(CheckTypeSize)
check_type_size(void* SIZEOF_VOID_PTR)
if("${SIZEOF_VOID_PTR}" STREQUAL "4")
  set(ARCH_32 1)
elseif("${SIZEOF_VOID_PTR}" STREQUAL "8")
  set(ARCH_64 1)
else()
  message(FATAL_ERROR "Unsupported architecture")
  return()
endif()

# Get ext includes
include_directories(src/liblt
  include
  include/FreeType)

# Get ext libraries
if(WINDOWS)

  file(GLOB_RECURSE EXTLIB extlib/win32/*.lib)
  set(LINK_LIBRARIES
      SFML::Graphics
      SFML::Audio
      SFML::Network
      SFML::System
      SFML::Window)

elseif(MAC)

  set(LINK_LIBRARIES
      freetype
      SFML::Graphics
      SFML::Audio
      SFML::Network
      SFML::System
      SFML::Window)

  link_directories(${PROJECT_SOURCE_DIR}/extbin/osx)

elseif(LINUX)

  set(LINK_LIBRARIES
      dl
      freetype
      SFML::Graphics
      SFML::Audio
      SFML::Network
      SFML::System
      SFML::Window)

  if(ARCH_32)

    set(LINK_LIBRARIES ${LINK_LIBRARIES})
    link_directories(${PROJECT_SOURCE_DIR}/extbin/linux32)

  elseif(ARCH_64)

    set(LINK_LIBRARIES ${LINK_LIBRARIES})
    link_directories(${PROJECT_SOURCE_DIR}/extbin/linux64)

  endif()

endif()

set(LINK_LIBRARIES ${LINK_LIBRARIES} ${OPENGL_LIBRARIES})

# Build shared libary
file(GLOB_RECURSE LIBLT_SRC src/liblt/*.cpp)
add_library(lt SHARED ${LIBLT_SRC})
target_sources(lt PRIVATE src/liblt/ThirdParty/glad.c)
target_link_libraries(lt ${LINK_LIBRARIES} ${EXTLIB})
configure_output_dir(lt)

# Post-build step to copy DLLs to bin folder
if (MSVC)
  file(GLOB_RECURSE RUNTIME_BINARIES extbin/win32/*.dll)
  foreach(X ${RUNTIME_BINARIES})
    add_custom_command (TARGET lt POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${X} $<TARGET_FILE_DIR:lt>)
  endforeach(X)
endif()

# Build binaries
set(TARGETS
    launch)

foreach(TARGET ${TARGETS})
  
  add_executable(${TARGET} ${PROJECT_SOURCE_DIR}/src/${TARGET}/${TARGET}.cpp src/resources.rc)
  target_link_libraries(${TARGET} ${LINK_LIBRARIES})
  configure_output_dir(${TARGET})

endforeach(TARGET)

# Link dynamic targets to the shared lib
foreach(TARGET ${TARGETS})

  target_link_libraries(${TARGET} lt)

endforeach(TARGET)

# LTE core unit tests (headless; link against the engine shared library)
add_subdirectory(tests)

# Treat warnings as errors for our own code
target_compile_options(lt PRIVATE -Werror)
foreach(TARGET ${TARGETS})
  target_compile_options(${TARGET} PRIVATE -Werror)
endforeach(TARGET)

# === Old-code revival demos (src/old/) — investigation targets, droppable ===
# Revived from Josh Parnell's original standalone executables. Kept as small,
# runnable demos of functionality the scripted apps don't exercise; see
# docs/old-code-investigation.md. Build: bin/ltheory_app, bin/audio_demo.
foreach(DEMO ltheory_app audio_demo)
  if(DEMO STREQUAL "ltheory_app")
    add_executable(ltheory_app ${PROJECT_SOURCE_DIR}/src/old/ltheory/ltheory-app.cpp)
  else()
    add_executable(audio_demo ${PROJECT_SOURCE_DIR}/src/old/demos/audio-demo.cpp)
  endif()
  target_link_libraries(${DEMO} ${LINK_LIBRARIES} lt)
  configure_output_dir(${DEMO})
  target_compile_options(${DEMO} PRIVATE -Werror)
endforeach(DEMO)

# Set target properties
if(WINDOWS)

  # No exception handling
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs")
  
  # Multi-threaded compilation
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

  # No libcmt warnings
  set_target_properties(lt PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libcmt")
  foreach(TARGET ${EXECUTABLES})
    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libcmt")
  endforeach(TARGET)

  # The updater requires administrator privileges to run
  # set_target_properties(updater PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'")

else()

  # Gold linker (deprecated/removed on modern toolchains; let CMake pick the default)
  if (LINUX)
    # set(CMAKE_LINKER "/usr/bin/gold")
  endif()

  # No exception handling
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")

  # Enable lots of warnings (without -Werror so modern compilers don't hard-fail)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-tautological-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy")

  # Stop on first error for faster iteration
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")

  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-conversion")

  # Enable SSE / SSE2
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")

endif()
