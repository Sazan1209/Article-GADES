#ifndef DISTANCE_FUNCS_HPP
#define DISTANCE_FUNCS_HPP

#include <armadillo>
#include <arrayfire.h>

inline arma::mat arma_dist_euclid(const arma::mat& a)
{
  arma::mat res(a.n_cols, a.n_cols, arma::fill::none);
  for (size_t i = 0; i < a.n_cols; ++i)
  {
    res.row(i) = arma::vecnorm(a.each_col() - a.col(i), 2, 0);
  }
  return res;
}

inline arma::mat arma_dist_l1(const arma::mat& a)
{
  arma::mat res(a.n_cols, a.n_cols, arma::fill::none);
  for (size_t i = 0; i < a.n_cols; ++i)
  {
    res.row(i) = arma::vecnorm(a.each_col() - a.col(i), 1, 0);
  }
  return res;
}

inline arma::mat arma_dist_pearson(const arma::mat& a)
{
  return arma::cor(a);
}

inline arma::mat arma_dist_cosine(const arma::mat& a)
{
  arma::mat res = a.t() * a;
  arma::vec norms = arma::sqrt(res.diag());
  res.each_col() /= norms;
  res.each_row() /= norms.t();
  return res;
}

// arma doesn't suppot kendall

inline static af::array square(const af::array& a)
{
  return a * a;
}

inline af::array af_eucl_dist1(const af::array& a)
{
  // int feat_len = a.dims(0); // Same as b.dims(0);
  int alen = a.dims(1);

  af::array dist_mat = af::constant(0, alen, alen, af_dtype::f64);
  for (int jj = 0; jj < alen; jj++)
  {
    af::array bvec = a(af::span, jj);
    af::array bvec_tiled = af::tile(bvec, 1, alen);
    af::array sad = af::sqrt(af::sum(square(bvec_tiled - a)));
    dist_mat(af::span, jj) = sad.T();
  }
  dist_mat.eval();
  af::sync();
  return dist_mat;
}

inline af::array af_l1_dist(const af::array& a)
{
  int alen = a.dims(1);

  af::array dist_mat = af::constant(0, alen, alen, af_dtype::f64);
  for (int jj = 0; jj < alen; jj++)
  {
    af::array bvec = a(af::span, jj);
    af::array bvec_tiled = af::tile(bvec, 1, alen);
    af::array sad = af::sum(af::abs(bvec_tiled - a));
    dist_mat(af::span, jj) = sad.T();
  }
  dist_mat.eval();
  af::sync();
  return dist_mat;
}

// More memory intensive than dist1, but faster
inline af::array af_eucl_dist2(const af::array& a)
{
  int feat_len = a.dims(0);
  int alen = a.dims(1);

  af::array a_mod = a;
  af::array b_mod = af::moddims(a, feat_len, 1, alen);

  af::array a_tiled = af::tile(a_mod, 1, 1, alen);
  af::array b_tiled = af::tile(b_mod, 1, alen, 1);

  af::array dist_mod = af::sqrt(af::sum(square(a_tiled - b_tiled)));
  af::array dist_mat = af::moddims(dist_mod, alen, alen);
  dist_mat.eval();
  af::sync();
  return dist_mat;
}

inline af::array af_pearson_dist(const af::array& a)
{
  int feat_len = a.dims(0);

  af::array mean_a = af::mean(a);
  af::array a_centered = a - af::tile(mean_a, feat_len, 1);
  af::array a_norm = af::sqrt(af::sum(square(a_centered), 0));
  a_centered /= af::tile(a_norm, feat_len, 1);

  af::array cov = af::matmul(a_centered, a_centered, AF_MAT_TRANS);
  cov.eval();
  af::sync();
  return cov;
}

inline af::array af_cosine_dist(const af::array& a)
{
  int feat_len = a.dims(0);

  af::array a_norm = af::sqrt(af::sum(square(a), 0));
  af::array a_centered = a / af::tile(a_norm, feat_len, 1);
  af::array cov = af::matmul(a_centered, a_centered, AF_MAT_TRANS);
  cov.eval();
  af::sync();
  return cov;
}

#endif  // DISTANCE_FUNCS_HPP
