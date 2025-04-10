#include <iostream>
#include <armadillo>
#include <arrayfire.h>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <string>
#include <complex>
#include "rapidcsv.hpp"
#include "distance_funcs.hpp"
#include "GADES.hpp"

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

void validate(int argc, char* argv[])
{
  if (argc != 7)
  {
    fprintf(stderr, "Expected 6 arguments, got %d\n", argc - 1);
    std::exit(1);
  }
  std::string method = argv[2];
  std::vector valid_methods = {"af_cpu", "af_oneapi", "af_opencl", "arma", "gades"};
  if (std::all_of(
        valid_methods.begin(), valid_methods.end(), [&](auto curr) { return method != curr; }))
  {
    std::string methods;
    for (auto curr : valid_methods)
    {
      methods += curr;
      methods += ", ";
    }
    fprintf(stderr, "Expected one of [%s] got \"%s\" instead\n", methods.c_str(), argv[2]);
    std::exit(1);
  }
  std::string metric = argv[4];
  std::vector valid_metrics = {"euclid", "pearson", "l1"};
  if (std::all_of(
        valid_metrics.begin(), valid_metrics.end(), [&](auto curr) { return metric != curr; }))
  {
    std::string metrics;
    for (auto curr : valid_metrics)
    {
      metrics += curr;
      metrics += ", ";
    }
    fprintf(stderr, "Expected one of [%s] got \"%s\" instead\n", metrics.c_str(), argv[4]);
    std::exit(1);
  }
}

int main(int argc, char* argv[])
{
  validate(argc, argv);

  std::string data_in = argv[1];
  std::string method = argv[2];
  int times = atoi(argv[3]);
  std::string metric = argv[4];
  std::string output = argv[5];
  int threads = atoi(argv[6]);

  try
  {
    if (method == "af_cpu")
    {
      af::setBackend(AF_BACKEND_CPU);
    }
    else if (method == "af_oneapi")
    {
      af::setBackend(AF_BACKEND_ONEAPI);
    }
    else if (method == "af_opencl")
    {
      af::setBackend(AF_BACKEND_OPENCL);
    }
  }
  catch (af::exception& e)
  {
    fprintf(stderr, "Caught exception when trying to set af backend\n");
    fprintf(stderr, "%s\n", e.what());
    throw;
  }

  rapidcsv::Document doc;

  try
  {
    doc = rapidcsv::Document(data_in, rapidcsv::LabelParams(0, 0));
  }
  catch (std::exception e)
  {
    fprintf(stderr, "Caught exception when trying to create csv reader for %s\n", data_in.c_str());
    fprintf(stderr, "%s\n", e.what());
  }
  // The input matrices are transposed
  int row_count = doc.GetColumnCount() - 1;
  int col_count = doc.GetRowCount() - 1;
  std::vector<double> data_vec;
  data_vec.reserve(col_count * row_count);
  for (int i = 1; i < col_count; ++i)
  {
    auto curr = doc.GetRow<double>(i);
    data_vec.insert(data_vec.end(), curr.begin(), curr.end());
  }
  const double* data = data_vec.data();

  std::vector<double> measurements;

  if (method == "af_cpu" || method == "af_oneapi" || method == "af_opencl")
  {
    af::array a = af::array(row_count, col_count, data);
    if (metric == "euclid")
    {
      measurements = iterate(times, [&]() { af_eucl_dist1(a); });
    }
    else if (metric == "pearson")
    {
      measurements = iterate(times, [&]() { af_pearson_dist(a); });
    }
    else
    {
      measurements = iterate(times, [&]() { af_l1_dist(a); });
    }
  }
  else if (method == "arma")
  {
    arma::mat a = arma::mat(data, row_count, col_count);
    if (metric == "euclid")
    {
      measurements = iterate(times, [&]() { arma_dist_euclid(a); });
    }
    else if (metric == "pearson")
    {
      measurements = iterate(times, [&]() { arma_dist_pearson(a); });
    }
    else
    {
      measurements = iterate(times, [&]() { arma_dist_l1(a); });
    }
  }
  else
  {
    MatrixView<const double> a = {.row_num = row_count, .col_num = col_count, .data = data};
    std::vector<double> res_data(col_count * col_count);
    MatrixView<double> res = {.row_num = col_count, .col_num = col_count, .data = res_data.data()};
    if (metric == "euclid")
    {
      std::abort();
    }
    else if (metric == "pearson")
    {
      std::abort();
    }
    else
    {
      measurements = iterate(times, [&]() { CalcDistanceL1(a, res, 0, threads); });
    }
  }
  arma::vec measure_vec(measurements);
  double mean = arma::mean(measure_vec);
  double max = arma::max(measure_vec);
  double stddev = arma::stddev(measure_vec);
  std::cout << mean << " " << stddev << " " << max;
  std::fstream out(output, std::fstream::out);
  for (size_t i = 0; i < measurements.size(); ++i)
  {
    out << measurements[i];
    if (i + 1 != measurements.size())
    {
      out << '\n';
    }
  }
  out.close();
  std::cout << std::endl;
}
