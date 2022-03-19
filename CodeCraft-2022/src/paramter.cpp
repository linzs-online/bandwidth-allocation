#include <random>
#include "../inc/paramter.h"


/**
 * @brief 并非标准的softmax,并未使用指数，算比例用的
 * @param start 
 * @param offValue 增量更新值
 */
void Paramerter::softmax(size_t start, float offValue) {
        float sum = std::accumulate(value.begin() + start, value.end(), 0.0f);
        for (size_t i = start; i < value.size(); i++) {
            if (offValue != 0.0f) {
                float tmp = value[i] + (value[i] / sum) * offValue;
                if (tmp < 0) {
                    offValue += tmp;
                    value[i] = 0.0f;

                } else {
                    value[i] = tmp;
                }
            } else {
                value[i] = value[i] / sum;
            }
        }
}


void Paramerter::normal(double mean, double stddev) {
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> rng{mean, stddev};
    for (size_t i = 0; i < value.size(); i++) {
        value[i] = std::abs(rng(gen));
    }
}