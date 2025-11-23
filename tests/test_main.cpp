#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Basic sanity tests
bool test_json_library() {
    json j;
    j["test"] = "value";
    j["number"] = 42;

    return j["test"] == "value" && j["number"] == 42;
}

int main() {
    std::cout << "Running basic tests..." << std::endl;

    if (!test_json_library()) {
        std::cerr << "JSON library test failed!" << std::endl;
        return 1;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
