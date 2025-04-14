#!/bin/bash

ROOT_FOLDER=$(realpath $1)
script="$ROOT_FOLDER"/scripts/Benchmarking/python_benchmarking.sh

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }

for method in "pandas" "pythonic"
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
            folder="${ROOT_FOLDER}"/results/GeneratedDense/${cells}_cells_${features}_features
            mkdir -p "$folder"

            for metric in "cosine" #"l1" "spearman" "kendall" "pearson"
            do
                if [[ $metric == "kendall" && $method == "pandas" ]]; then continue; fi
                name="benchmark_"${method}_${metric}_${cells}x${features}
                output="$folder"/_${method}_${metric}.csv
                logs="${ROOT_FOLDER}/logs/$name.out"
                sbatch --job-name=$name -o "$logs" --cpus-per-task=24 -D $(dirname $script) "$script" --metric $metric --input "$input" --times 25 --output "$output" --method $method ||
                { echo "Couldn't run sbatch for some reason"; exit 1; }
            done
        done
    done
done
