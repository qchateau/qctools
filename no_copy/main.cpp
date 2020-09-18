#include <vector>

#include "no_copy.hpp"

#define DOES_NOT_COMPILE(x) x

template <typename... Args>
using nc_vector = nc::no_copy<std::vector<Args...>>;

void test_nc()
{
    nc_vector<int> nc_data{1, 2, 3, 4};

    DOES_NOT_COMPILE(nc_vector<int> nc_data_bad(nc_data));
    nc_vector<int> nc_data2(std::move(nc_data));

    DOES_NOT_COMPILE(nc_data = nc_data2);
    nc_data = std::move(nc_data2);

    nc_vector<int> cloned_data = nc_data.clone();
}

void test_std_to_nc()
{
    std::vector<int> data{1, 2, 3, 4};

    DOES_NOT_COMPILE(nc_vector<int> nc_data_bad(data));
    nc_vector<int> nc_data(std::move(data));

    DOES_NOT_COMPILE(nc_data = data);
    nc_data = std::move(data);
}

int main(int, char**)
{
    test_nc();
    test_std_to_nc();
    return 0;
}