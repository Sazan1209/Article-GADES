#! /bin/bash

#SBATCH --partition=starwind
#SBATCH --cpus-per-task=24
#SBATCH --mem=20G

Rscript R_benchmarking.R "$@"
