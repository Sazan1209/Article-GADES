#!/bin/bash


ROOT_FOLDER=../../
dataset=$1
experiment_name=$2

output_folder=${experiment_name}
for method in "pythonic" #"pandas"
do
    for metric in "kendall"
    do
        mkdir -p $output_folder
        echo $method $metric
        python3 python_benchmarking.py --num_threads 24 --metric $metric --input $dataset --times 10 --output $output_folder/dense_${method}_${metric}.csv --method $method
    done
done
