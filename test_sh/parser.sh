#!/bin/bash
# the absolute path of "outfilepath" in auto.sh
result_dir="/home/jbyao/auto_result"
echo "-------------------------------------------------------------------------------------------------------"
echo "Figure 6: The performance of in-memory MioDB using micro-benchmarks"
echo "the order is 1KB, 4KB, 16KB, 64KB"
echo "random write            latency       throughput"
cat $result_dir/db_bench_random_1KB.out | grep "fillrandom  "
cat $result_dir/db_bench_random_4KB.out | grep "fillrandom  "
cat $result_dir/db_bench_random_16KB.out | grep "fillrandom  "
cat $result_dir/db_bench_random_64KB.out | grep "fillrandom  "

echo ""
echo "sequential write        latency       throughput"
cat $result_dir/db_bench_seq_1KB.out | grep "fillseq  "
cat $result_dir/db_bench_seq_4KB.out | grep "fillseq  "
cat $result_dir/db_bench_seq_16KB.out | grep "fillseq  "
cat $result_dir/db_bench_seq_64KB.out | grep "fillseq  "

echo ""
echo "random read             latency       throughput"
cat $result_dir/db_bench_random_1KB.out | grep "readrandom  "
cat $result_dir/db_bench_random_4KB.out | grep "readrandom  "
cat $result_dir/db_bench_random_16KB.out | grep "readrandom  "
cat $result_dir/db_bench_random_64KB.out | grep "readrandom  "

echo ""
echo "sequential read         latency       throughput"
cat $result_dir/db_bench_seq_1KB.out | grep "readseq  "
cat $result_dir/db_bench_seq_4KB.out | grep "readseq  "
cat $result_dir/db_bench_seq_16KB.out | grep "readseq  "
cat $result_dir/db_bench_seq_64KB.out | grep "readseq  "

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Table 1: Different costs in MioDB"
echo "Interval Stall: 0"
tmp=$(cat $result_dir/db_bench_random_4KB.out | grep "stall")
for line in $tmp
do
    line=$line
done
echo "Cumulative Stall: $line us"
echo "Deserialization: 0"
tmp=$(cat $result_dir/db_bench_random_4KB.out | grep "dump")
for line in $tmp
do
    line=$line
done
echo "Flushing Time: $line us"
tmp=$(cat $result_dir/db_bench_random_4KB.out | grep "wa")
for line in $tmp
do
    line=$line
done
total=81920000000
line=`expr ${line} + ${total}`
wa=$(echo "scale=1; $line/$total" | bc | awk '{printf "%.1f", $0}')
echo "Write Amplification: $wa X"

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Figure 7: The throughput (KIOPS) of YCSB workloads running on KV stores deployed in an in-memory mode"
echo "the order is Load, A, B, C, D, E, F"
echo "value size : 1KB"
cat $result_dir/YCSB_1KB.out | grep "loading"
cat $result_dir/YCSB_1KB.out | grep "all op"

echo ""
echo "value size : 4KB"
cat $result_dir/YCSB_4KB.out | grep "loading"
cat $result_dir/YCSB_4KB.out | grep "all op"

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Table 2: Tail latencies of workload A in YCSB"
echo "value size : 4KB"
python3 /home/jbyao/ycsb_mio/tail.py auto_result_3/YCSB_4KB_tail_latency.out

echo ""
echo "value size : 1KB"
python3 /home/jbyao/ycsb_mio/tail.py auto_result_3/YCSB_1KB_tail_latency.out

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Figure 10: The random read/write performance varies with the size of dataset in MioDB"
echo "the order is Load, 40GB, 80GB, 120GB, 160GB, 200GB"
echo "random read             latency       throughput"
cat $result_dir/dateset_size_40GB.out | grep "readrandom  "
cat $result_dir/dateset_size_80GB.out | grep "readrandom  "
cat $result_dir/dateset_size_120GB.out | grep "readrandom  "
cat $result_dir/dateset_size_160GB.out | grep "readrandom  "
cat $result_dir/dateset_size_200GB.out | grep "readrandom  "

echo ""
echo "random write            latency       throughput"
cat $result_dir/dateset_size_40GB.out | grep "fillrandom  "
cat $result_dir/dateset_size_80GB.out | grep "fillrandom  "
cat $result_dir/dateset_size_120GB.out | grep "fillrandom  "
cat $result_dir/dateset_size_160GB.out | grep "fillrandom  "
cat $result_dir/dateset_size_200GB.out | grep "fillrandom  "

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Figure 11: The WA ratio varies with the size of dataset"
filename="$result_dir/dateset_size_40GB.out"
total=40960000000
GETWA() {
    tmp=$(cat $filename | grep "wa")
    for line in $tmp
    do
        line=$line
    done
    line=`expr ${line} + ${total}`
    wa=$(echo "scale=1; $line/$total" | bc | awk '{printf "%.1f", $0}')
}
filename="$result_dir/dateset_size_40GB.out"
total=40960000000
GETWA
echo "40GB: $wa X"

filename="$result_dir/dateset_size_80GB.out"
total=81920000000
echo "80GB: $wa X"

filename="$result_dir/dateset_size_120GB.out"
total=122880000000
GETWA
echo "120GB: $wa X"

filename="$result_dir/dateset_size_160GB.out"
total=163840000000
GETWA
echo "160GB: $wa X"

filename="$result_dir/dateset_size_200GB.out"
total=204800000000
GETWA
echo "200GB: $wa X"

echo ""
echo "-------------------------------------------------------------------------------------------------------"
echo "Figure 12: The latency and throughput of MemTable flushing vary with the size of MemTable in the DRAM."
echo "flushing latency"
tmp=$(cat $result_dir/memtable_size_64MB.out | grep "dump")
for line in $tmp
do
    line=$line
done
echo "64MB: $line us"
tmp=$(cat $result_dir/memtable_size_128MB.out | grep "dump")
for line in $tmp
do
    line=$line
done
echo "128MB: $line us"
tmp=$(cat $result_dir/memtable_size_256MB.out | grep "dump")
for line in $tmp
do
    line=$line
done
echo "256MB: $line us"
tmp=$(cat $result_dir/memtable_size_512MB.out | grep "dump")
for line in $tmp
do
    line=$line
done
echo "512MB: $line us"

echo ""
echo "the order is 64MB, 128MB, 256MB, 512MB"
echo "random write            latency       throughput"
cat $result_dir/memtable_size_64MB.out | grep "fillrandom  "
cat $result_dir/memtable_size_128MB.out | grep "fillrandom  "
cat $result_dir/memtable_size_256MB.out | grep "fillrandom  "
cat $result_dir/memtable_size_512MB.out | grep "fillrandom  "

echo ""
echo "random read            latency       throughput"
cat $result_dir/memtable_size_64MB.out | grep "readrandom  "
cat $result_dir/memtable_size_128MB.out | grep "readrandom  "
cat $result_dir/memtable_size_256MB.out | grep "readrandom  "
cat $result_dir/memtable_size_512MB.out | grep "readrandom  "