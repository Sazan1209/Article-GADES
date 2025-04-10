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

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }

for method in "gades" "arma" "af_cpu"
do
  for cells in "10" "100" "1000" "10000"
  do
    for features in "10" "100" "1000" "10000" "100000"
    do
      num_elements=$(( cells * features ))
      if [[ $num_elements > 1000000 ]]
      then
        continue
      fi

      input="${ROOT_FOLDER}"/Datasets/Generated/${cells}_cells_${features}_features.csv
      folder="${ROOT_FOLDER}"/results/SingleThreadGeneratedDense/${cells}_cells_${features}_features/
      mkdir -p "$folder"
      for metric in "l1" # "euclid" "pearson"
      do
        name="benchmark_"${method}_${metric}_${cells}x${features}
        output="$folder"/_${method}_${metric}.csv
        logs="${ROOT_FOLDER}"/logs/$name
        sbatch --job-name=$name --partition=$PARTITION --cpus-per-task=1 -o "$logs" "$script" "$input" $method 25 $metric "$output" 1 || { echo "Couldn't run sbatch for some reason"; exit 1; }
      done
    done
  done
done
