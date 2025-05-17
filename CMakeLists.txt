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

# Create library target
add_library(conduit ${LIBRARY_SOURCES})
target_include_directories(conduit 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Create example executable
add_executable(simple_get examples/simple_get.c)
target_link_libraries(simple_get PRIVATE conduit)

# Installation rules
install(TARGETS conduit
    EXPORT conduit-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES include/conduit.h DESTINATION include)
install(EXPORT conduit-targets
    FILE conduit-targets.cmake
    NAMESPACE Conduit::
    DESTINATION lib/cmake/conduit
)