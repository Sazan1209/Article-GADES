#!/bin/bash

ROOT_FOLDER=$1


for method in "GPU" "CPU"
do
    for cells in "10" "100" "1000" "10000"
    do
        for features in "10" "100" "1000" "10000" "100000"
        do
            num_elements=$(( cells * features ))

            if [[ $num_elements > 10000000  || $num_elements < 100000 ]]
            then
                continue
            fi
            
            echo $cells, $features
            for sparsity in 0.5 0.75 0.9 0.95 0.99
            do
                input=${ROOT_FOLDER}/Datasets/GeneratedSparse/${cells}_cells_${features}_features/$sparsity.mtx
                for metric in "kendall"
                do
                    times=25

                    folder=${ROOT_FOLDER}/results/GeneratedSparse/${cells}_cells_${features}_features/$sparsity
                    mkdir -p $folder
                    Rscript test.R $input $method $times $metric 5000 $folder TRUE temp
                done
            done
        done	
    done
done
