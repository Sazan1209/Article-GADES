#!/bin/bash


ROOT_FOLDER=$1

for method in "GPU"
do
    for cells in "10" "100" "1000" "10000"
    do
        for features in "10" "100" "1000" "10000" "100000"
        do
            num_elements=$(( cells * features ))

            if [[ $num_elements > 10000000 ]]
            then
                continue
            else
                echo $cells, $features
                input=${ROOT_FOLDER}/Datasets/Generated/${cells}_cells_${features}_features.csv
                for metric in "kendall" #"euclidean" # "pearson"
                do
                    folder=${ROOT_FOLDER}results/GeneratedDenseMemoryUsage/${cells}_cells_${features}_features/
                    mkdir -p $folder

                    Rscript test.R $input $method 1 $metric 500 $folder FALSE test_dump TRUE
                done
            fi
        done	
    done
done
