#! /bin/bash

for dataset in B_CD8T B_T Camp CellLines Chen FibrocardATAC FibrocardRNA HLCA_aorta HLCA_lung HLCA_marrow HSC PBMC5K PBMC_all TCells
do
  ./scripts/Benchmarking/runners/cpu_cxx_real_run_benchmark.sh . starwind $dataset
  ./scripts/Benchmarking/runners/cpu_py_real_run_benchmark.sh . $dataset
  ./scripts/Benchmarking/runners/cpu_r_real_run_benchmark.sh . $dataset
done
