#pragma once

#include <memory>
#include <vector>
#include <numeric>


class Paramerter {
public:
    using Ptr = std::shared_ptr<Paramerter>;
    std::vector<float> value;

    Paramerter() {};
    void softmax(size_t start = 0, float offValue = 0);
    void normal(double mean = 0, double stddev = 1);
};

