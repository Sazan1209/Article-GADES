#! /bin/bash

#SBATCH --mem=20G

./buildRelease/GadesCScript "$@"
