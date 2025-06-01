#!/bin/bash

ROOT_FOLDER=$(realpath "$1")
PARTITION=$2
DATASET=$3

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
        input="${ROOT_FOLDER}"/Datasets/Real/$DATASET.mtx
        folder="${ROOT_FOLDER}"/results/Real/
        mkdir -p "$folder"
        for metric in "spearman" #"cosine" "l1" #"euclid" "pearson"
        do
          if [[ $metric == "spearman" && $method != "gades" ]]; then continue; fi
          name=${method}_${metric}_${DATASET}"_benchmark"
          output="$folder"/_${method}_${metric}_${DATASET}.csv
          logs="${ROOT_FOLDER}"/logs/$name
          sbatch --job-name=$name --partition=$PARTITION --time=1-00:00:00 --cpus-per-task=24 -o "$logs" "$script" "$input" $method 25 $metric "$output" 24 || { echo "Couldn't run sbatch for some reason"; exit 1; }
        done
done
