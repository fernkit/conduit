# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name, version and language
project(conduit VERSION 0.1.0 DESCRIPTION "Simple HTTP client library" LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define library source files
set(LIBRARY_SOURCES
    src/conduit.c
    src/http_client.c
    src/failures.c
    src/json_parser.c
)

# Define public headers
set(PUBLIC_HEADERS
    include/conduit.h
    include/http_client.h
    include/failures.h
    include/json_parser.h
)

# Create library target
add_library(conduit ${LIBRARY_SOURCES})
target_include_directories(conduit 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add version information to the library
set_target_properties(conduit PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Create example executable
add_executable(simple_post examples/simple_post.c)
target_link_libraries(simple_post PRIVATE conduit)


# Installation rules
install(TARGETS conduit
    EXPORT conduit-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install all public headers
install(FILES ${PUBLIC_HEADERS} DESTINATION include/conduit)

# Generate and install package configuration files
include(CMakePackageConfigHelpers)

# Generate the config file that includes the exports
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/conduit-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/conduit-config.cmake
    INSTALL_DESTINATION lib/cmake/conduit
)

# Generate the version file for the config file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/conduit-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the configuration files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/conduit-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/conduit-config-version.cmake
    DESTINATION lib/cmake/conduit
)

# Install the export set for use with the install-tree
install(EXPORT conduit-targets
    FILE conduit-targets.cmake
    NAMESPACE Conduit::
    DESTINATION lib/cmake/conduit
)

# Configure and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/conduit.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/conduit.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/conduit.pc DESTINATION lib/pkgconfig)