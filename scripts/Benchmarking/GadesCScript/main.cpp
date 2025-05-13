#include <iostream>
#include <armadillo>
#include <arrayfire.h>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <string>
#include <complex>
#include <filesystem>
#include "mm.hpp"
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

struct Matrix
{
  int row_count;
  int col_count;
  std::vector<double> data_vec;
};

struct SparseMatrix
{
  int row_count;
  int col_count;
  std::vector<double> vals;
  std::vector<int64_t> rows;
  std::vector<uint32_t> cols;
};

SparseMatrix read_mtx(std::filesystem::path data_in)
{
  typedef MtxReader<int, double> reader_t;
  typedef typename reader_t::coo_type coo_t;
  typedef typename reader_t::csr_type csr_t;
  typedef typename coo_t::entry_type entry_t;

  // read matrix as coo
  reader_t reader(data_in);
  coo_t coo = reader.read_coo();
  std::sort(coo.entries.begin(), coo.entries.end(), [](auto a, auto b) {
    return a.j < b.j || (a.j == b.j && a.i < b.i);
  });
  std::vector<double> vals(coo.nnz());
  std::vector<int64_t> rows(coo.nnz());
  std::vector<uint32_t> col_offsets(coo.num_cols() + 1, 0);
  for (size_t i = 0; i < coo.nnz(); ++i)
  {
    vals[i] = coo.entries[i].e;
    rows[i] = coo.entries[i].i;
    ++col_offsets[coo.entries[i].j + 1];
  }
  for (size_t i = 1; i <= coo.num_cols(); ++i)
  {
    col_offsets[i] += col_offsets[i - 1];
  }
  return SparseMatrix{
    .row_count = coo.num_rows(),
    .col_count = coo.num_cols(),
    .vals = std::move(vals),
    .rows = std::move(rows),
    .cols = std::move(col_offsets)};
}

Matrix read_csv(std::filesystem::path data_in)
{
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
  return Matrix{.row_count = row_count, .col_count = col_count, .data_vec = std::move(data_vec)};
}


af::array read_data_af(std::filesystem::path data_in)
{
  if (data_in.extension() == ".mtx")
  {
    typedef MtxReader<int, double> reader_t;
    typedef typename reader_t::coo_type coo_t;
    typedef typename reader_t::csr_type csr_t;
    typedef typename coo_t::entry_type entry_t;

    // read matrix as coo
    reader_t reader(data_in);
    coo_t coo = reader.read_coo();
    std::vector<double> vals(coo.nnz());
    std::vector<int> rows(coo.nnz());
    std::vector<int> cols(coo.nnz());
    for (size_t i = 0; i < coo.nnz(); ++i)
    {
      vals[i] = coo.entries[i].e;
      rows[i] = coo.entries[i].i;
      rows[i] = coo.entries[i].j;
    }
    return af::dense(af::sparse(
      coo.num_cols(),
      coo.num_rows(),
      coo.nnz(),
      vals.data(),
      rows.data(),
      cols.data(),
      af_dtype::f64));
  }
  else
  {
    auto [row_count, col_count, data_vec] = read_csv(data_in);
    return af::array(row_count, col_count, data_vec.data());
  }
}

arma::mat read_data_arma(std::filesystem::path data_in)
{
  if (data_in.extension() == ".mtx")
  {
    fprintf(stderr, "Sparse format for armadillo not supported, got %s as input", data_in.c_str());
    exit(1);
  }
  else
  {
    auto [row_count, col_count, data_vec] = read_csv(data_in);
    return arma::mat(data_vec.data(), row_count, col_count);
  }
}

std::vector<double> iterate_gades(auto a, size_t threads, std::string metric, size_t times)
{
  std::vector<double> res_data(a.col_num * a.col_num);
  MatrixView<double> res = {.row_num = a.col_num, .col_num = a.col_num, .data = res_data.data()};
  std::vector<double> measurements;
  if (metric == "euclid")
  {
    std::abort();
  }
  else if (metric == "pearson")
  {
    std::abort();
  }
  else if (metric == "l1")
  {
    measurements = iterate(times, [&]() { CalcDistanceL1(a, res, 0, threads); });
  }
  else if (metric == "cosine")
  {
    measurements = iterate(times, [&]() { CalcDistanceCosine(a, res, 0, threads); });
  }
  else if (metric == "spearman")
  {
    if constexpr (std::is_same_v<typeof(a), MatrixView<const double>>)
    {
      measurements = iterate(times, [&]() { CalcDistanceSpearman(a, res, 0, threads); });
    }
  }
  return measurements;
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

  std::vector<double> measurements;

  if (method == "af_cpu" || method == "af_oneapi" || method == "af_opencl")
  {
    af::array a = read_data_af(data_in);
    if (metric == "euclid")
    {
      measurements = iterate(times, [&]() { af_eucl_dist1(a); });
    }
    else if (metric == "pearson")
    {
      measurements = iterate(times, [&]() { af_pearson_dist(a); });
    }
    else if (metric == "l1")
    {
      measurements = iterate(times, [&]() { af_l1_dist(a); });
    }
    else if (metric == "cosine")
    {
      measurements = iterate(times, [&]() { af_cosine_dist(a); });
    }
  }
  else if (method == "arma")
  {
    arma::mat a = read_data_arma(data_in);
    if (metric == "euclid")
    {
      measurements = iterate(times, [&]() { arma_dist_euclid(a); });
    }
    else if (metric == "pearson")
    {
      measurements = iterate(times, [&]() { arma_dist_pearson(a); });
    }
    else if (metric == "l1")
    {
      measurements = iterate(times, [&]() { arma_dist_l1(a); });
    }
    else if (metric == "cosine")
    {
      measurements = iterate(times, [&]() { arma_dist_cosine(a); });
    }
  }
  else
  {
    if (std::filesystem::path(data_in).extension() == ".csv")
    {
      auto [row_count, col_count, data] = read_csv(data_in);
      MatrixView<const double> a = {
        .row_num = static_cast<size_t>(row_count),
        .col_num = static_cast<size_t>(col_count),
        .data = data.data()};
      measurements = iterate_gades(a, threads, metric, times);
    }
    else
    {
      auto [row_count, col_count, vals, rows, col_offsets] = read_mtx(data_in);
      MatrixViewCSC<const double, const int64_t, const uint32_t> a{
        .vals = vals.data(),
        .rows = rows.data(),
        .col_offsets = col_offsets.data(),
        .col_num = static_cast<size_t>(col_count),
        .row_num = static_cast<size_t>(row_count),
      };
      measurements = iterate_gades(a, threads, metric, times);
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
