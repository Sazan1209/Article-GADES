#include <iostream>
#include <armadillo>
#include <arrayfire.h>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include "mm.hpp"
#include "rapidcsv.hpp"
#include "distance_funcs.hpp"
#include "GADES.hpp"
#include "utility.hpp"

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
  std::vector valid_metrics = {"euclid", "pearson", "l1", "cosine", "spearman"};
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

Config get_config(int argc, char* argv[])
{
  validate(argc, argv);
  return {
    .input = argv[1],
    .output = argv[5],
    .iter_count = static_cast<size_t>(atoi(argv[3])),
    .worker_count = static_cast<size_t>(atoi(argv[6])),
    .method = argv[2],
    .metric = argv[4],
  };
}

void output(const std::vector<double>& measurements, Config conf)
{
  arma::vec measure_vec(measurements);
  double mean = arma::mean(measure_vec);
  double max = arma::max(measure_vec);
  double stddev = arma::stddev(measure_vec);
  std::cout << mean << " " << stddev << " " << max;
  std::fstream out(conf.output, std::fstream::out);
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

std::vector<double> bench_dense(Config conf)
{
  rapidcsv::Document doc;
  try
  {
    doc = rapidcsv::Document(conf.input, rapidcsv::LabelParams(0, 0));
  }
  catch (std::exception e)
  {
    fprintf(
      stderr, "Caught exception when trying to create csv reader for %s\n", conf.input.c_str());
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

  std::vector<double> measurements;
  if (conf.metric == "af_cpu")
  {
    af::array a = af::array(row_count, col_count, data_vec.data());
    if (conf.metric == "euclid")
    {
      measurements = iterate(conf.iter_count, [&]() { af_eucl_dist1(a); });
    }
    else if (conf.metric == "pearson")
    {
      measurements = iterate(conf.iter_count, [&]() { af_pearson_dist(a); });
    }
    else if (conf.metric == "l1")
    {
      measurements = iterate(conf.iter_count, [&]() { af_l1_dist(a); });
    }
    else if (conf.metric == "cosine")
    {
      measurements = iterate(conf.iter_count, [&]() { af_cosine_dist(a); });
    }
  }
  else if (conf.method == "arma")
  {
    arma::mat a(data_vec.data(), row_count, col_count);
    if (conf.metric == "euclid")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_euclid(a); });
    }
    else if (conf.metric == "pearson")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_pearson(a); });
    }
    else if (conf.metric == "l1")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_l1(a); });
    }
    else if (conf.metric == "cosine")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_cosine(a); });
    }
  }
  else
  {
    MatrixView<const double> a{
      .row_num = static_cast<size_t>(row_count),
      .col_num = static_cast<size_t>(col_count),
      .data = data_vec.data(),
    };
    std::vector<double> res_data(col_count * col_count);
    MatrixView res{
      .row_num = static_cast<size_t>(col_count),
      .col_num = static_cast<size_t>(col_count),
      .data = res_data.data(),
    };
    if (conf.metric == "euclid" || conf.metric == "pearson")
    {
      std::abort();
    }
    else if (conf.metric == "l1")
    {
      measurements =
        iterate(conf.iter_count, [&]() { CalcDistanceL1(a, res, 0, conf.worker_count); });
    }
    else if (conf.metric == "cosine")
    {
      measurements =
        iterate(conf.iter_count, [&]() { CalcDistanceCosine(a, res, 0, conf.worker_count); });
    }
    else if (conf.metric == "spearman")
    {
      measurements =
        iterate(conf.iter_count, [&]() { CalcDistanceSpearman(a, res, 0, conf.worker_count); });
    }
  }
  return measurements;
}


typedef MtxReader<int, double> reader_t;
typedef typename reader_t::coo_type coo_t;
typedef typename coo_t::entry_type entry_t;

std::vector<double> undense(const coo_t& coo)
{
  std::vector<double> res(coo.num_cols() * coo.num_rows(), 0.0);
  for (entry_t e : coo.entries)
  {
    res[e.i * coo.num_cols() + e.j] = e.e;
  }
  return res;
}

std::vector<double> bench_sparse(Config conf)
{

  // read matrix as coo
  reader_t reader(conf.input);
  coo_t coo = reader.read_coo();
  size_t col_num = coo.num_rows();
  size_t row_num = coo.num_cols();


  std::vector<double> measurements;
  if (conf.metric == "af_cpu")
  {
    std::vector<double> data_vec = undense(coo);
    af::array a = af::array(row_num, col_num, data_vec.data());
    if (conf.metric == "euclid")
    {
      measurements = iterate(conf.iter_count, [&]() { af_eucl_dist1(a); });
    }
    else if (conf.metric == "pearson")
    {
      measurements = iterate(conf.iter_count, [&]() { af_pearson_dist(a); });
    }
    else if (conf.metric == "l1")
    {
      measurements = iterate(conf.iter_count, [&]() { af_l1_dist(a); });
    }
    else if (conf.metric == "cosine")
    {
      measurements = iterate(conf.iter_count, [&]() { af_cosine_dist(a); });
    }
  }
  else if (conf.method == "arma")
  {
    std::vector<double> data_vec = undense(coo);
    arma::mat a(data_vec.data(), row_num, col_num);
    if (conf.metric == "euclid")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_euclid(a); });
    }
    else if (conf.metric == "pearson")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_pearson(a); });
    }
    else if (conf.metric == "l1")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_l1(a); });
    }
    else if (conf.metric == "cosine")
    {
      measurements = iterate(conf.iter_count, [&]() { arma_dist_cosine(a); });
    }
  }
  else
  {
    std::vector<double> vals(coo.nnz());
    std::vector<int64_t> rows(coo.nnz());
    std::vector<uint32_t> col_offsets(coo.num_rows() + 1, 0);

    std::sort(coo.entries.begin(), coo.entries.end(), entry_t::by_ij);
    for (size_t i = 0; i < coo.nnz(); ++i)
    {
      vals[i] = coo.entries[i].e;
      rows[i] = coo.entries[i].j;
      ++col_offsets[coo.entries[i].i + 1];
    }
    for (size_t i = 1; i <= coo.num_rows(); ++i)
    {
      col_offsets[i] += col_offsets[i - 1];
    }
    MatrixViewCSC<const double, const int64_t, const uint32_t> a{
      .vals = vals.data(),
      .rows = rows.data(),
      .col_offsets = col_offsets.data(),
      .col_num = col_num,
      .row_num = row_num,
    };
    std::vector<double> res_data(col_num * col_num);
    MatrixView res{
      .row_num = col_num,
      .col_num = col_num,
      .data = res_data.data(),
    };


    if (conf.metric == "euclid" || conf.metric == "pearson" || conf.metric == "spearman")
    {
      std::abort();
    }
    else if (conf.metric == "l1")
    {
      measurements =
        iterate(conf.iter_count, [&]() { CalcDistanceL1(a, res, 0, conf.worker_count); });
    }
    else if (conf.metric == "cosine")
    {
      measurements =
        iterate(conf.iter_count, [&]() { CalcDistanceCosine(a, res, 0, conf.worker_count); });
    }
  }
  return measurements;
}

int main(int argc, char* argv[])
{
  Config conf = get_config(argc, argv);
  try
  {
    if (conf.method == "af_cpu")
    {
      af::setBackend(AF_BACKEND_CPU);
    }
    else if (conf.method == "af_oneapi")
    {
      af::setBackend(AF_BACKEND_ONEAPI);
    }
    else if (conf.method == "af_opencl")
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
  std::vector<double> measurements;
  if (conf.input.extension() == ".mtx")
  {
    measurements = bench_sparse(conf);
  }
  else
  {
    measurements = bench_dense(conf);
  }
  output(measurements, conf);
}
