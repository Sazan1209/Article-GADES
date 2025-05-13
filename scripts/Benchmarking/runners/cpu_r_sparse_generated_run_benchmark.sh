#!/bin/bash

ROOT_FOLDER=$(realpath "$1")

script="$ROOT_FOLDER"/scripts/Benchmarking/R_benchmarking.sh

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }
export {OMP_NUM_THREADS,OPENBLAS_NUM_THREADS}=24

for method in "amap" "factoextra"
do
  for cells in "10" "100" "1000" "10000"
  do
    for features in "10" "100" "1000" "10000" "100000"
    do
      for sparsity in "0.5" "0.75" "0.9" "0.95" "0.99"
      do
        num_elements=$(( cells * features ))

        if [[ $num_elements > 10000000 || $num_elements < 100000 ]]
        then
          continue
        fi
        input="${ROOT_FOLDER}"/Datasets/GeneratedSparse/${cells}_cells_${features}_features/$sparsity.mtx
        folder="${ROOT_FOLDER}"/results/GeneratedSparse/${cells}_cells_${features}_features
        mkdir -p "$folder"
        for metric in "spearman" "cosine" "manhattan" #"kendall"
        do
          if [[ $metric == "cosine" && $method == "factoextra" ]]; then continue; fi
          name=${method}_${metric}_${cells}x${features}x${sparsity}"_benchmark"
          output="$folder"/${sparsity}_${method}_${metric}.csv
          logs="${ROOT_FOLDER}/logs/$name.out"
          if [[ $metric == "manhattan" ]]; then output="$folder"/${sparsity}_${method}_l1.csv; fi
          logs="${ROOT_FOLDER}"/logs/$name
          sbatch --job-name=$name  --cpus-per-task=24 -o "$logs" -D $(dirname $script) "$script" "$input" $method 25 $metric "$output" TRUE FALSE 24
        done
      done
    done
  done
done
