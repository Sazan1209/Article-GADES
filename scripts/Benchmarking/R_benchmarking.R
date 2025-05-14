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
threads = strtoi(args[8])

if (profile) {
    library(profmem)
    library(ps)
}

if (sparse) {
    data <- readMM(datain)
} else {
    data <- as.matrix(read.table(datain, header=T, row.names = 1, sep=","))
}



measurements <- numeric(times)

for (i in 1:times) {
    st_t <- as.numeric(Sys.time()) * 1000000

    if (method == 'amap') {
        if (metric == 'cosine'){
          distMatrix_mtrx <- as.matrix(Dist(data, method='pearson', nbproc=threads))
        } else {
          distMatrix_mtrx <- as.matrix(Dist(data, method=metric, nbproc=threads))
        }
    } else if (method == 'factoextra') {
        if (sparse) {
            data <- as.matrix(data)
        }

        distMatrix_mtrx <- as.matrix(get_dist(data, method = metric))
    }

    end_time <- as.numeric(Sys.time()) * 1000000
    measurements[i] <- end_time - st_t
    print(distMatrix_mtrx.nrow, distMatrix_mtrx.ncol)
    gc()
}

write.table(measurements, output, sep=',')

