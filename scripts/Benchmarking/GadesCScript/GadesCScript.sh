#! /bin/bash

#SBATCH --partition=comet
#SBATCH --cpus-per-task=24
#SBATCH --mem=20G

./buildRelease/GadesCScript "$@"
