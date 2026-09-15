cmake_minimum_required(VERSION 3.16)
project(BFlex VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)

option(BFLEX_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BFLEX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BFLEX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if (BFLEX_ENABLE_TSAN AND (BFLEX_ENABLE_ASAN OR BFLEX_ENABLE_UBSAN))
    message(FATAL_ERROR "TSAN cannot be combined with ASAN/UBSAN in this project configuration")
endif()

set(BFLEX_SANITIZER_COMPILE_FLAGS "")
set(BFLEX_SANITIZER_LINK_FLAGS "")

if (BFLEX_ENABLE_ASAN)
    list(APPEND BFLEX_SANITIZER_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
    list(APPEND BFLEX_SANITIZER_LINK_FLAGS -fsanitize=address)
endif()
if (BFLEX_ENABLE_UBSAN)
    list(APPEND BFLEX_SANITIZER_COMPILE_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
    list(APPEND BFLEX_SANITIZER_LINK_FLAGS -fsanitize=undefined)
endif()
if (BFLEX_ENABLE_TSAN)
    list(APPEND BFLEX_SANITIZER_COMPILE_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
    list(APPEND BFLEX_SANITIZER_LINK_FLAGS -fsanitize=thread)
endif()

if (BFLEX_SANITIZER_COMPILE_FLAGS)
    add_compile_options(${BFLEX_SANITIZER_COMPILE_FLAGS})
    add_link_options(${BFLEX_SANITIZER_LINK_FLAGS})
endif()

add_library(flex
    src/search_algorithm/path_finder.cpp
    src/search_algorithm/apply.cpp
    src/search_algorithm/bfs_step.cpp
    src/runtime/group_checkpoint.cpp
    src/runtime/solution_output.cpp
    src/runtime/thread_pool.cpp
    src/engine/engine.cpp
)
set_target_properties(flex PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(flex PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(flex PRIVATE -Wall -Wextra -Wpedantic)

add_executable(example_dual_edge_flip_flop_3state
    examples/dual_edge_flip_flop_3state.cpp
)

target_link_libraries(example_dual_edge_flip_flop_3state PRIVATE flex)

option(BFLEX_BUILD_PYTHON "Build Python bindings" OFF)
if (BFLEX_BUILD_PYTHON)
    add_subdirectory(pyflex)
endif()

enable_testing()
add_executable(flex_tests
    tests/test_basic.cpp
)
target_link_libraries(flex_tests PRIVATE flex)
add_test(NAME flex_tests COMMAND flex_tests)

include(CMakePackageConfigHelpers)
install(TARGETS flex EXPORT flexTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT flexTargets
    FILE BFlexTargets.cmake
    NAMESPACE BFlex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BFlex
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BFlexConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BFlexConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BFlex
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BFlexConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BFlexConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BFlexConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BFlex
)
