#!/bin/bash

ROOT_FOLDER=$1
script="$ROOT_FOLDER"/scripts/Benchmarking/python_benchmarking.sh

[[ -a $script ]] || { echo "Couldn't find script at ${script}"; exit 1; }

for method in "pandas" "scipy"
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
            
            for metric in "spearman" "kendall" "pearson"
            do
                if [[ $metric == "kendall" && $method == "pandas" ]]; then continue; fi
                name="benchmark_"${method}_${metric}_${cells}x${features}
                output="$folder"/${method}_${metric}
                sbatch --job-name=$name -o=$name.out "$script" --metric $metric --input "$input" --times 25 --output "$output" --method $method ||
                { echo "Couldn't run sbatch for some reason"; exit 1; }
            done
        done	
    done
done
