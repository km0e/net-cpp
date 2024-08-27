/**
 * @file random.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_TEST_RANDOM
#  define XSL_TEST_RANDOM
#  include <random>
#  include <vector>
class UniformDistributionGenerator {
private:
  std::mt19937 gen;

public:
  UniformDistributionGenerator() : gen(std::random_device()()) {}
  std::vector<int> generate(int n, int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    std::vector<int> res;
    for (int i = 0; i < n; i++) {
      res.push_back(dis(gen));
    }
    return res;
  }
};
#endif
