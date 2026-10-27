cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)
option(VISUAL_SIDECHAIN_BUILD_TESTS "Build the CTest suite" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.c
    src/core/audio-ring.h
    src/core/audio-ring.c
    src/core/fft.h
    src/core/fft.c
    src/core/spectrum.h
    src/core/spectrum.c
    src/core/envelope.h
    src/core/envelope.c
    src/core/onset.h
    src/core/onset.c
    src/core/analyzer.h
    src/core/analyzer.c
    src/core/modulation.h
    src/core/modulation.c
    src/core/baseline.h
    src/core/baseline.c
    src/obs/audio-tap.h
    src/obs/audio-tap.c
    src/obs/modulator-filter.h
    src/obs/modulator-filter.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(VISUAL_SIDECHAIN_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
