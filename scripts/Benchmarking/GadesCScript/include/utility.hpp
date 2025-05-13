#pragma once

#include <string_view>
#include <filesystem>
#include <cstdio>

template <typename Func>
std::vector<double> iterate(int times, Func function)
{
  std::vector<double> measurements(times);
  for (int i = 0; i < times; ++i)
  {
    auto begin = std::chrono::high_resolution_clock::now();
    function();
    auto end = std::chrono::high_resolution_clock::now();
    measurements[i] = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
  }
  return measurements;
}

struct Config
{
  std::filesystem::path input;
  std::filesystem::path output;
  size_t iter_count;
  size_t worker_count;
  std::string_view method;
  std::string_view metric;
};
