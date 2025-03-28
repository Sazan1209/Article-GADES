#!/bin/bash


dataset=$1
experiment_name=$2

output_folder=${experiment_name}
for method in "CPU" "GPU" "amap" "factoextra"
do
    for metric in "kendall"
    do
        for density in "dense" "sparse"
        do
            if [[ $density == dense ]]
            then
                dense=FALSE
            else
                dense=TRUE
            fi
            mkdir -p $output_folder
            times=10
            echo Rscript test.R $dataset $method $times $metric 5000 $output_folder/$density $dense
            if [[ $density == "sparse" ]]
            then
                if [[ $method == "amap" || $method == "factoextra" ]]
                then
                    continue
                fi
            fi

            Rscript test.R $dataset $method $times $metric 5000 $output_folder/$density $dense $output_folder/${density}_matrix
        done
    done
done

