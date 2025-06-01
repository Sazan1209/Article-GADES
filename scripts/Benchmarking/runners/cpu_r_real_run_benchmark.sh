#!/bin/bash

ROOT_FOLDER=$(realpath "$1")
DATASET=$2

script="$ROOT_FOLDER"/scripts/Benchmarking/R_benchmarking.sh

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }
export {OMP_NUM_THREADS,OPENBLAS_NUM_THREADS}=24

for method in "amap" "factoextra"
do
        input="${ROOT_FOLDER}"/Datasets/Real/$DATASET.mtx
        folder="${ROOT_FOLDER}"/results/Real
        mkdir -p "$folder"
        for metric in "spearman" "cosine" "manhattan" #"kendall"
        do
          if [[ $metric == "cosine" && $method == "factoextra" ]]; then continue; fi
          name=${method}_${metric}_${DATASET}"_benchmark"
          output="$folder"/${method}_${metric}_${DATASET}.csv
          logs="${ROOT_FOLDER}/logs/$name.out"
          if [[ $metric == "manhattan" ]]; then output="$folder"/${method}_l1_${DATASET}.csv; fi
          logs="${ROOT_FOLDER}"/logs/$name
          sbatch --job-name=$name  --cpus-per-task=24 -o "$logs" -D $(dirname $script) --time=1-00:00:00 "$script" "$input" $method 25 $metric "$output" TRUE FALSE 24
        done
done
