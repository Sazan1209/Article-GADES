#include <gtest/gtest.h>
#include "distance_funcs.hpp"
#include <span>
#include <cmath>

static double calcPearson(std::span<double> a, std::span<double> b)
{
  double avg_a = 0.0;
  double avg_b = 0.0;
  for (size_t i = 0; i < a.size(); ++i)
  {
    avg_a += a[i];
    avg_b += b[i];
  }
  avg_a /= a.size();
  avg_b /= a.size();
  double cov = 0.0;
  double std_a = 0.0;
  double std_b = 0.0;
  for (size_t i = 0; i < a.size(); ++i)
  {
    cov += (a[i] - avg_a) * (b[i] - avg_b);

    double tmp = (a[i] - avg_a);
    std_a += tmp * tmp;

    tmp = (b[i] - avg_b);
    std_b += tmp * tmp;
  }
  std_a = sqrt(std_a);
  std_b = sqrt(std_b);
  return cov / (std_a * std_b);
}

static double calcCosine(std::span<double> a, std::span<double> b)
{
  double cov = 0.0;
  double std_a = 0.0;
  double std_b = 0.0;
  for (size_t i = 0; i < a.size(); ++i)
  {
    cov += a[i] * b[i];

    std_a += a[i] * a[i];
    std_b += b[i] * b[i];
  }
  std_a = sqrt(std_a);
  std_b = sqrt(std_b);
  return cov / (std_a * std_b);
}

static double calcEuclid(std::span<double> a, std::span<double> b)
{
  double res = 0.0;
  for (size_t i = 0; i < a.size(); ++i)
  {
    double tmp = (a[i] - b[i]);
    res += tmp * tmp;
  }
  return sqrt(res);
}

// This is shit, but I'm lazy

constexpr size_t row_num = 10;
constexpr size_t col_num = 10;

void init(double* matrix)
{
  std::mt19937 gen(21); // Standard mersenne_twister_engine seeded with rd()
  std::normal_distribution<> dis(-20.0, 20.0);
  for (size_t i = 0; i < row_num * col_num; ++i)
  {
    matrix[i] = dis(gen);
  }
}

TEST(Arma, Pearson)
{
  double data[row_num * col_num];
  init(data);
  arma::mat a = arma::mat(data, row_num, col_num);
  arma::mat res = arma_dist_pearson(a);
  for (size_t i = 0; i < col_num; ++i)
  {
    for (size_t j = 0; j < col_num; ++j)
    {
      std::span a(data + i * row_num, row_num);
      std::span b(data + j * row_num, row_num);
      EXPECT_NEAR(calcPearson(a, b), res(i, j), 1e-9);
    }
  }
}

TEST(Arma, Euclid)
{
  double data[row_num * col_num];
  init(data);
  arma::mat a = arma::mat(data, row_num, col_num);
  arma::mat res = arma_dist_euclid(a);
  for (size_t i = 0; i < col_num; ++i)
  {
    for (size_t j = 0; j < col_num; ++j)
    {
      std::span a(data + i * row_num, row_num);
      std::span b(data + j * row_num, row_num);
      EXPECT_NEAR(calcEuclid(a, b), res(i, j), 1e-9);
    }
  }
}

TEST(Arma, Cosine)
{
  double data[row_num * col_num];
  init(data);
  arma::mat a = arma::mat(data, row_num, col_num);
  arma::mat res = arma_dist_cosine(a);
  for (size_t i = 0; i < col_num; ++i)
  {
    for (size_t j = 0; j < col_num; ++j)
    {
      std::span a(data + i * row_num, row_num);
      std::span b(data + j * row_num, row_num);
      EXPECT_NEAR(calcCosine(a, b), res(i, j), 1e-9);
    }
  }
}

// TEST(AF, Pearson)
// {
//   double data[row_num * col_num];
//   init(data);
//   af::array a = af::array(row_num, col_num, data);
//   af::array res = af_pearson_dist(a);
//   for (size_t i = 0; i < col_num; ++i)
//   {
//     for (size_t j = 0; j < col_num; ++j)
//     {
//       std::span a(data + i * row_num, row_num);
//       std::span b(data + j * row_num, row_num);
//       EXPECT_NEAR(calcPearson(a, b), res(i, j).scalar<double>(), 1e-9);
//     }
//   }
// }

TEST(AF, Euclid)
{
  double data[row_num * col_num];
  init(data);
  af::array a = af::array(row_num, col_num, data);
  af::array res = af_eucl_dist1(a);
  for (size_t i = 0; i < col_num; ++i)
  {
    for (size_t j = 0; j < col_num; ++j)
    {
      std::span a(data + i * row_num, row_num);
      std::span b(data + j * row_num, row_num);
      EXPECT_NEAR(calcEuclid(a, b), res(i, j).scalar<double>(), 1e-9);
    }
  }
}

// TEST(AF, Cosine)
// {
//   double data[row_num * col_num];
//   init(data);
//   af::array a = af::array(row_num, col_num, data);
//   af::array res = af_cosine_dist(a);
//   for (size_t i = 0; i < col_num; ++i)
//   {
//     for (size_t j = 0; j < col_num; ++j)
//     {
//       std::span a(data + i * row_num, row_num);
//       std::span b(data + j * row_num, row_num);
//       EXPECT_NEAR(calcCosine(a, b), res(i, j).scalar<double>(), 1e-9);
//     }
//   }
// }
