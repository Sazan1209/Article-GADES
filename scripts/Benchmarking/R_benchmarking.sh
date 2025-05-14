#! /bin/bash

#SBATCH --partition=starwind
#SBATCH --mem=20G

Rscript R_benchmarking.R "$@"
