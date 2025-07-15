# Test CMakeLists.txt for Conduit C++ Library

add_executable(test_basic_cpp test_basic.cpp)
target_link_libraries(test_basic_cpp PRIVATE conduit-cpp)

# Add test
add_test(NAME BasicCppTests COMMAND test_basic_cpp)
