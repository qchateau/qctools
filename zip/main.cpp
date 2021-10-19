#include <iostream>
#include <string>
#include <vector>

#include "zip.hpp"

int main(int, char**)
{
    using qctools::zip;

    // lvalue ref
    std::vector<int> v1 = {1, 2, 3};
    std::vector<std::string> v2 = {"a", "b"};
    for (const auto& pair : zip(v1, v2)) {
        std::cerr << std::get<0>(pair) << ", " << std::get<1>(pair) << std::endl;
    }

    // rvalue ref
    for (const auto& pair : zip(std::vector<int>{1, 2, 3}, std::vector<std::string>{"a", "b"})) {
        std::cerr << std::get<0>(pair) << ", " << std::get<1>(pair) << std::endl;
    }

    // const ref
    const std::vector<int> cv1 = {1, 2, 3};
    const std::vector<std::string> cv2 = {"a", "b"};
    for (const auto& pair : zip(cv1, cv2)) {
        std::cerr << std::get<0>(pair) << ", " << std::get<1>(pair) << std::endl;
    }
    return 0;
}
