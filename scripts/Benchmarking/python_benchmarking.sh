#! /bin/bash

#SBATCH --partition=starwind
#SBATCH --mem=20G

python3 ./python_benchmarking.py "$@"
