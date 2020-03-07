# ##############################################################################
# PROJECT
# ##############################################################################
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(trm VERSION 1.0.0)

option(STATIC_ANALYSIS "Use static analysis tools" FALSE)
option(OPTIMIZE_DEBUG "Enable compile time optimization for debug build" TRUE)
option(PROFILER "Enable deuging profiler" FALSE)
option(HPC "Preforms rendering using multiple threads" TRUE)
set(TRM_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(TRM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ##############################################################################
# BUILD MODE
# ##############################################################################
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      "Debug"
      CACHE STRING "Build Mode" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
  if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
              ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
  endif()
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D__DEBUG__")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
if(OPTIMIZE_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3")
endif()
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")
endif()

# ##############################################################################
# STATIC ANALYSIS
# ##############################################################################
if(STATIC_ANALYSIS OR CMAKE_BUILD_TYPE STREQUAL "Release")
  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "Path to clang tidy exe")
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
  endif()
  find_program(CPPCHECK_EXE NAME "cppcheck" DOC "Path to cppcheck exe")
  if(CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK "${CPPCHECK_EXE}" "--force")
  endif()
  find_program(CPPLINT_EXE "cpplint")
  if(CPPLINT_EXE)
    set(CMAKE_CXX_CPPLINT "${CPPLINT_EXE}")
  endif()
endif()

# ##############################################################################
# Fetch Macro
# ##############################################################################
find_package(Git REQUIRED)
macro(clone_git REPO_NAME)
  string(TOUPPER ${REPO_NAME} MODULE_NAME)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${REPO_NAME}/.git)
    execute_process(
      COMMAND
        git submodule update --init --recursive --
        ${CMAKE_CURRENT_SOURCE_DIR}/${REPO_NAME})
  elseif(NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required for submodules ${MODULE_NAME}")
  else()
    set(${MODULE_NAME}_FOUND TRUE)
  endif()
endmacro()
macro(load_submodule REPO_NAME)
  clone_git(${REPO_NAME})
  string(TOUPPER ${REPO_NAME} MODULE_NAME)
  if(${MODULE_NAME}_FOUND)
    message(STATUS "FOUND PACKAGE: ${MODULE_NAME} / ${REPO_NAME}")
    message(
      STATUS "Module path: ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${REPO_NAME}")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${REPO_NAME}
                     EXCLUDE_FROM_ALL)
  else()
    message(STATUS "DID not find ${MODULE_NAME")
  endif()
endmacro()

# ##############################################################################
# DEPENDENCIES
# ##############################################################################
load_submodule(glm)
load_submodule(fmt)
load_submodule(json)

# ##############################################################################
# EXECUTABLE
# ##############################################################################
set(SOURCES
    src/main.cpp
    src/bar.cpp
    src/img.cpp
    src/prof.cpp
    src/sdf.cpp
    src/argparse.cpp
    src/scene.cpp)
add_executable(trm ${SOURCES})
set_target_properties(
  trm PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS OFF)
target_link_libraries(trm glm::glm fmt::fmt nlohmann_json::nlohmann_json)

# ##############################################################################
# THREAD COMPUTE
# ##############################################################################
find_package(OpenMP QUIET)
# find_package(OpenACC QUIET)
if(OpenACC_CXX_FOUND AND HPC)
  target_link_libraries(trm ${OpenACC_CXX_LIBRARIES})
  target_compile_options(trm PUBLIC ${OpenACC_CXX_FLAGS})
  target_link_options(trm PUBLIC ${OpenACC_CXX_FLAGS})
  message(STATUS "Using OpenACC ${OpenACC_CXX_FLAGS}")
elseif(OpenMP_CXX_FOUND AND HPC)
  target_link_libraries(trm ${OpenMP_CXX_LIBRARIES})
  target_compile_options(trm PUBLIC ${OpenMP_CXX_FLAGS})
  message(STATUS "Using OpenMP ${OpenMP_CXX_FLAGS}")
elseif(HPC)
  message(STATUS "Failed to load OpenACC or OpenMP")
endif()

# ##############################################################################
# PROFILING
# ##############################################################################
if(PROFILER)
  message(STATUS "Enabled profiling")
  target_compile_definitions(trm PUBLIC -DENABLE_PROF)
endif()
