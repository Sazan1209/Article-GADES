#!/bin/bash

ROOT_FOLDER=$(realpath "$1")

script="$ROOT_FOLDER"/scripts/Benchmarking/R_benchmarking.sh

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }

for method in "amap" "factoextra"
do
  for cells in "10" "100" "1000" "10000"
  do
    for features in "10" "100" "1000" "10000" "100000"
    do
      num_elements=$(( cells * features ))

      if [[ $num_elements > 10000000 ]]
      then
        continue
      fi
      input="${ROOT_FOLDER}"/Datasets/Generated/${cells}_cells_${features}_features.csv
      folder="${ROOT_FOLDER}"/results/GeneratedDense/${cells}_cells_${features}_features/
      mkdir -p "$folder"
      for metric in "spearman" #"cosine" "manhattan" "kendall"
      do
        if [[ $metric == "cosine" && $method == "factoextra" ]]; then continue; fi
        name="benchmark_"${method}_${metric}_${cells}x${features}
        output="$folder"/_${method}_${metric}.csv
        logs="${ROOT_FOLDER}"/logs/$name
        sbatch --job-name=$name  --cpus-per-task=24 -o "$logs" -D $(dirname $script) "$script" "$input" $method 25 $metric "$output" FALSE FALSE 24
      done
    done
  done
done
