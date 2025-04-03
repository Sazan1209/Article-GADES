library(amap)
library(Matrix)
library(factoextra)
library(glue)

args = commandArgs(trailingOnly=TRUE)
datain = args[1]
method = args[2]
times = strtoi(args[3])
metric = args[4]
output = args[5]
sparse = as.logical(args[6])
profile = as.logical(args[7])

if (profile) {
    library(profmem)
    library(ps)
}
print(glue('Reading table. Output folder will be {output}_{method}_{metric}.csv'))

if (sparse) {
    data <- readMM(datain)
} else {
    data <- t(as.matrix(read.table(datain, header=T, row.names = 1, sep=",")))
}
print('Completed reading')

measurements <- numeric(times)

for (i in 1:times) {
    st_t <- as.numeric(Sys.time()) * 1000000

    if (method == 'amap') {
        print('Calc dist')
        distMatrix_mtrx <- as.matrix(Dist(t(data), method=metric, nbproc=24))
    } else if (method == 'factoextra') {
        if (!sparse) {
            data <- as.matrix(data)
        }

        distMatrix_mtrx <- as.matrix(get_dist(t(data), method = metric))
        print('Factoextra')
    }
    end_time <- as.numeric(Sys.time()) * 1000000
    measurements[i] <- end_time - st_t

    if (profile) {
        p <- profmem_end()
        if (method == 'amap' || method == 'factoextra') {
            delta <- object.size(distMatrix_mtrx)
        } else {
            delta <- 0
        }
        print(p)
        sum_bytes <- sum(p$bytes, na.rm=T)
        end <- as.numeric(ps::ps_memory_info()['rss'][1])
        print('Memory usage')
        print(c(sum_bytes, delta))

        delta_manual <- 0
        print(method)
        memories[i, 1] <- sum_bytes + delta_manual
        memories[i, 2] <- delta
        memories[i, 3] <- delta_manual
        memories[i, 4] <- end - start
    }
    
    gc()
}

if (profile){
    write.table(memories, glue("{output}_{method}_{metric}_memory.csv", sep=','))
}
write.table(measurements, output, sep=',')
