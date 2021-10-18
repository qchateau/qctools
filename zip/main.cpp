#include <iostream>
#include <string>
#include <vector>

#include "zip.hpp"

int main(int, char**)
{
    std::vector<int> v1 = {1, 2, 3};
    std::vector<std::string> v2 = {"a", "b"};
    for (const auto& pair : qctools::zip(v1, v2)) {
        std::cerr << std::get<0>(pair) << ", " << std::get<1>(pair) << std::endl;
    }
    return 0;
}
