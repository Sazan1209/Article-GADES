#!/bin/bash

ROOT_FOLDER=$(realpath $1)
DATASET=$2
script="$ROOT_FOLDER"/scripts/Benchmarking/python_benchmarking.sh


[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }
export {OMP_NUM_THREADS,OPENBLAS_NUM_THREADS}=24

for method in "scikit" #"pythonic" "pandas"
do

        input="${ROOT_FOLDER}"/Datasets/Real/$DATASET.mtx
        folder="${ROOT_FOLDER}"/results/Real
        mkdir -p "$folder"

        for metric in "spearman" "cosine" "l1" #"spearman" "kendall" "pearson"
        do
          if [[ $metric == "spearman" && $method == "scikit" ]]; then continue; fi
          name=${method}_${metric}_${DATASET}"_benchmark"
          output="$folder"/${method}_${metric}_${DATASET}.csv
          logs="${ROOT_FOLDER}/logs/$name.out"
          sbatch --job-name=$name -o "$logs" --cpus-per-task=24 --time=1-00:00:00 -D $(dirname $script) "$script" --metric $metric --input "$input" --times 25 --output "$output" --method $method ||
          { echo "Couldn't run sbatch for some reason"; exit 1; }
        done
done
