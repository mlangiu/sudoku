cmake_minimum_required(VERSION 3.20)
project(sudoku-solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# helpful for IDEs/linters
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt auto tools (safe to enable; if Qt not found these have no effect)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME}
  src/test.cpp # later: main.cpp
  src/board.hpp
  src/solver.hpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Put build outputs under the build dir by configuration
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

# --- Dependencies (vcpkg manifest / toolchain provided by presets) ---
# Options
option(ENABLE_QT "Build with Qt GUI" OFF)
option(ENABLE_CBC "Build with CBC MILP solver" OFF)

# Make CMake preprocessor defines match exactly
if(ENABLE_QT)
    add_compile_definitions(ENABLE_QT)
endif()
if(ENABLE_CBC)
    add_compile_definitions(ENABLE_CBC)
endif()

# Include vcpkg toolchain (if not automatically set)
# This is usually passed via CMAKE_TOOLCHAIN_FILE in presets
# Example: -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# Tell vcpkg which features to enable
if(DEFINED ENV{VCPKG_FEATURE_FLAGS})
    # preserve existing feature flags
    set(_vcpkg_flags $ENV{VCPKG_FEATURE_FLAGS})
else()
    set(_vcpkg_flags "")
endif()

if(ENABLE_QT)
    list(APPEND _vcpkg_flags "ENABLE_QT")
endif()
if(ENABLE_CBC)
    list(APPEND _vcpkg_flags "ENABLE_CBC")
endif()

# Pass flags to vcpkg
set(ENV{VCPKG_FEATURE_FLAGS} "${_vcpkg_flags}")



# Qt6 (optional on platforms where GUI isn't required)
find_package(Qt6 COMPONENTS Widgets QUIET)
if (TARGET Qt6::Widgets)
  message(STATUS "Found Qt6::Widgets - enabling GUI.")
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
  # optional convenience if available
  if (COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
  endif()
else()
  message(STATUS "Qt6::Widgets not available; building headless (OK for Android/CI).")
endif()

# Eigen (header-only)
find_package(Eigen3 3.4 REQUIRED NO_MODULE)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

# COIN-OR CBC (robust lookup; ok to be absent on some targets)
find_package(Cbc CONFIG QUIET)
if (TARGET Cbc::Cbc)
  set(CBC_TARGET Cbc::Cbc)
elseif (TARGET coinor::cbc)
  set(CBC_TARGET coinor::cbc)
endif()

if (CBC_TARGET)
  message(STATUS "Linking against CBC via target ${CBC_TARGET}")
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CBC_TARGET})
else()
  find_library(CBC_LIB NAMES Cbc cbc coinor_cbc)
  if (CBC_LIB)
    message(STATUS "Found CBC library at ${CBC_LIB}")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CBC_LIB})
  else()
    message(WARNING "CBC not found. Solver will fall back to backtracking (install coin-or-cbc via vcpkg if needed).")
  endif()
endif()

