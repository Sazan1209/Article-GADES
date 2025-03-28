#!/bin/bash

ROOT_FOLDER=$1
for cells in "10" "100" "1000" "10000"
do
	for features in "10" "100" "1000" "10000" "100000"
	do
		num_elements=$(( cells * features ))
        if [[ $num_elements > 10000000 ]]
		then
			continue
        fi
        for num_batches in "1" "2" "5" "10" "20" "100"
        do
            batch_size=$(( cells / num_batches ))

            if [[ $batch_size -gt $cells ]]
            then
                continue
            fi
			echo $cells, $features, $batch_size
			input=$ROOT_FOLDER/Datasets/Generated/${cells}_cells_${features}_features.csv
			for metric in "kendall" #"euclidean" # "pearson"
			do
				folder=$ROOT_FOLDER/results/GeneratedDenseBatch_size/${cells}_cells_${features}_features/${batch_size}/
				mkdir -p $folder
                echo Rscript test.R $input GPU 10 $metric ${batch_size} $folder FALSE out FALSE

				Rscript test.R $input GPU 10 $metric ${batch_size} $folder FALSE out FALSE
			done
        done
	done	
done
