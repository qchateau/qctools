#include <string>
#include <tuple>

#include "to_struct.hpp"

struct S {
    int a;
    std::string b;
    double c;
};

int main(int, char**)
{
    auto tup = std::make_tuple(1, "toto", 2.0);
    auto s = qctools::to_struct<S>(tup);
    return 0;
}
