# Unit tests

# Note: For now, we'll create a placeholder test.
# In a full implementation, you would use a testing framework like Google Test

add_executable(basic_test
    test_main.cpp
)

target_include_directories(basic_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(basic_test PRIVATE
    nlohmann_json::nlohmann_json
)

# Register test
add_test(NAME BasicTest COMMAND basic_test)
