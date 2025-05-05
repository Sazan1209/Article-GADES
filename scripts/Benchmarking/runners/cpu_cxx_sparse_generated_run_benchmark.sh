#!/bin/bash

ROOT_FOLDER=$(realpath "$1")
PARTITION=$2

march=($(srun --partition=$2 gcc -march=native -Q --help=target | grep march))
march=${march[1]}
echo "Determined march for partition ${PARTITION} is ${march}"

cd "$ROOT_FOLDER"/scripts/Benchmarking/GadesCScript
{
  cmake -S . -B buildRelease -D GADES_MARCH=${march} &&
  cmake --build buildRelease --config Release &&
  cmake --install buildRelease --prefix "$PWD" --config Release
} || { echo "Failed to compile script"; exit 1; }

script="$ROOT_FOLDER"/scripts/Benchmarking/GadesCScript/GadesCScript.sh
export {OMP_NUM_THREADS,OPENBLAS_NUM_THREADS}=24

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }

for method in "gades" #"af_cpu" "arma"
do
  for cells in "10" "100" "1000" "10000"
  do
    for features in "10" "100" "1000" "10000" "100000"
    do
      for sparsity in "0.5" "0.75" "0.9" "0.95" "0.99"
      do
        num_elements=$(( cells * features ))
        if [[ $num_elements > 10000000 ]]
        then
          continue
        fi
        input="${ROOT_FOLDER}"/Datasets/GeneratedSparse/${cells}_cells_${features}_features/$sparsity.mtx
        folder="${ROOT_FOLDER}"/results/GeneratedSparse/${cells}_cells_${features}_features
        mkdir -p "$folder"
        for metric in "cosine" "l1"  # "spearman" "euclid" "pearson"
        do
          name=${method}_${metric}_${cells}x${features}x${sparsity}"_benchmark"
          output="$folder"/${sparsity}_${method}_${metric}.csv
          logs="${ROOT_FOLDER}"/logs/$name
          sbatch --job-name=$name --partition=$PARTITION --cpus-per-task=24 -o "$logs" "$script" "$input" $method 25 $metric "$output" 24 || { echo "Couldn't run sbatch for some reason"; exit 1; }
        done
      done
    done
  done
done
