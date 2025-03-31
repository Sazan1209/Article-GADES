#! /bin/bash

#SBATCH --partition=comet
#SBATCH --cpus-per-task=24
#SBATCH --mem=20G

cd $(realpath "${0%/*}")

./bin/GadesCScript "$@"
