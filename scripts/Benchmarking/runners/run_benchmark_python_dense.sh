#!/bin/bash

ROOT_FOLDER=$1

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
            echo $cells, $features
            input=${ROOT_FOLDER}/Datasets/Generated/${cells}_cells_${features}_features.csv
            for metric in "kendall"
            do
                folder=${ROOT_FOLDER}/results/GeneratedDense/${cells}_cells_${features}_features/
                mkdir -p $folder
                python3 python_benchmarking.py --num_threads 24 --metric $metric --input $input --times 25 --output ${folder}_${method}_${metric}.csv --method $method
            done
        done	
    done
done
