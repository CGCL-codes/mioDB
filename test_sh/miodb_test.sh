#!/bin/bash

# save .log LOG CURRENT MANIFEST...
db_path="/mnt/persist-memory/pmem_yjb_1/dbbench"
# output file dir
outfilepath="/home/jbyao/miodb/test"
# output file
outfile="$outfilepath/result.out"
# db_bench binary path
bench_path="/home/jbyao/miodb/build"
# total size of workload (Byte)
test_size=81920000000
# value size (Byte)
size="4096"
# evaluate multi sizes of value once excution
value_array=(1024 4096 16384 65536)
# write KV num, test_size / size
write_key_num="20000000"
# read KV num
read_key_num="1000000"
# test type in db_bench
test_type="fillrandom,stats,readrandom,stats"
# disable compression
comp_ratio="1"
# theoretical KV num in penultimate layer
bench_keys_per_datatable="2097152"
# dram node in numa
numa_dram_node=0
# nvm node in numa
numa_nvm_node=2
# nvm next node in numa, we can use -1 to disable this
numa_nvm_next_node=-1
# the size of memtable in dram (Byte)
write_buffer_size=67108864

# use RUN_ONE_TEST to start a db_bench test
RUN_ONE_TEST() {
    if [ -d "$db_path" ];then
        echo "Clean db path!"
        rm -rf $db_path/*
    fi
    parameters="--num=$write_key_num \
                --value_size=$size \
                --benchmarks=$test_type \
                --reads=$read_key_num \
                --compression_ratio=$comp_ratio \
                --db=$db_path \
                --keys_per_datatable=$bench_keys_per_datatable \
                --dram_node=$numa_dram_node \
                --nvm_node=$numa_nvm_node \
                --nvm_next_node=$numa_nvm_next_node \
                --write_buffer_size=$write_buffer_size
                "
    cmd="$bench_path/db_bench $parameters >> $outfile"
    echo $cmd > "$outfile"
    echo $cmd
    eval $cmd
}

# RUN_VALUE_TEST support to evaluate different value size test in sequence
RUN_VALUE_TEST() {
    if [ ! -d "$outfilepath" ];then
        echo "Make output file path!"
        mkdir $outfilepath
    fi
    for value in ${value_array[@]}; do
        size="$value"
        write_key_num="`expr $test_size / $size`"
        outfile="$outfilepath/rwrr-${size}.out"
        test_type="fillrandom,stats,readrandom,stats"
        RUN_ONE_TEST
        sleep 5
    done

    for value in ${value_array[@]}; do
        size="$value"
        write_key_num="`expr $test_size / $size`"
        outfile="$outfilepath/fillseq-${size}.out"
        test_type="fillseq,stats"
        RUN_ONE_TEST
        sleep 5
    done

    for value in ${value_array[@]}; do
        size="$value"
        write_key_num="`expr $test_size / $size`"
        outfile="$outfilepath/readseq-${size}.out"
        test_type="fillrandom,stats,readseq,stats"
        RUN_ONE_TEST
        sleep 5
    done
}

RUN_ONE_TEST
#RUN_VALUE_TEST
