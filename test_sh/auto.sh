# save .log LOG CURRENT MANIFEST...
db_path="/mnt/persist-memory/nvm"
# output file dir
outfilepath="/home/jbyao/auto_result"
# output file
outfile="$outfilepath/result.out"
# db_bench binary path
bench_path="/home/jbyao/mioDB/build"
# value size (Byte)
size="4096"
# write KV num, test_size / size
write_key_num="20000000"
# read KV num
read_key_num="1000000"
# test type in db_bench
test_type="fillrandom,stats,readrandom,stats"
# disable compression
comp_ratio="1"
# dram node in numa
numa_dram_node=0
# nvm node in numa
numa_nvm_node=4
# nvm next node in numa, we can use -1 to disable this
numa_nvm_next_node=-1
# the size of memtable in dram (Byte)
write_buffer_size=67108864
# the absolute path of excutable ycsbc
ycsbc_path="/home/jbyao/ycsb_mio/ycsbc"
# the absolute path of ycsb input
input_path="/home/jbyao/ycsb_mio/input"

CLEAN_DB_PATH() {
    if [ -d "$db_path" ];then
        echo "Clean db path!"
        rm -rf $db_path
    fi
}

CREATE_OUTPUT_FILE_PATH() {
    if [ ! -d "$outfilepath" ];then
        echo "Make output file path!"
        mkdir $outfilepath
    fi
}

# use RUN_ONE_TEST to start a db_bench test
RUN_ONE_TEST() {
    CLEAN_DB_PATH
    parameters="--num=$write_key_num \
                --value_size=$size \
                --benchmarks=$test_type \
                --reads=$read_key_num \
                --compression_ratio=$comp_ratio \
                --db=$db_path \
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

CREATE_OUTPUT_FILE_PATH

echo "------------db_bench------------"
echo "------1KB random write/read-----"
write_key_num="80000000"
size="1024"
test_type="fillrandom,readrandom"
outfile="$outfilepath/db_bench_random_1KB.out"
RUN_ONE_TEST

echo "------4KB random write/read-----"
write_key_num="20000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
outfile="$outfilepath/db_bench_random_4KB.out"
RUN_ONE_TEST

echo "------16KB random write/read-----"
write_key_num="5000000"
size="16384"
test_type="fillrandom,readrandom"
outfile="$outfilepath/db_bench_random_16KB.out"
RUN_ONE_TEST

echo "------64KB random write/read-----"
write_key_num="1250000"
size="65536"
test_type="fillrandom,readrandom"
outfile="$outfilepath/db_bench_random_64KB.out"
RUN_ONE_TEST

echo "------1KB sequential write/read-----"
write_key_num="80000000"
size="1024"
test_type="fillseq,readseq"
outfile="$outfilepath/db_bench_seq_1KB.out"
RUN_ONE_TEST

echo "------4KB sequential write/read-----"
write_key_num="20000000"
size="4096"
test_type="fillseq,readseq"
outfile="$outfilepath/db_bench_seq_4KB.out"
RUN_ONE_TEST

echo "------16KB sequential write/read-----"
write_key_num="5000000"
size="16384"
test_type="fillseq,readseq"
outfile="$outfilepath/db_bench_seq_16KB.out"
RUN_ONE_TEST

echo "------64KB sequential write/read-----"
write_key_num="1250000"
size="65536"
test_type="fillseq,readseq"
outfile="$outfilepath/db_bench_seq_64KB.out"
RUN_ONE_TEST

echo "------------YCSB------------"
echo "-----1KB YCSB performance-----"
CLEAN_DB_PATH
outfile="$outfilepath/YCSB_1KB.out"
cmd="$ycsbc_path $input_path/1KB_busy >> $outfile"
echo $cmd
echo $cmd > "$outfile"
eval $cmd

echo "-----4KB YCSB performance-----"
CLEAN_DB_PATH
outfile="$outfilepath/YCSB_4KB.out"
cmd="$ycsbc_path $input_path/4KB_busy >> $outfile"
echo $cmd
echo $cmd > "$outfile"
eval $cmd

echo "-----1KB YCSB tail latency-----"
CLEAN_DB_PATH
outfile="$outfilepath/YCSB_1KB_tail_latency.out"
cmd="$ycsbc_path $input_path/1KB_busy_tail_latency >> $outfile"
echo $cmd
echo $cmd > "$outfile"
eval $cmd

echo "-----4KB YCSB tail latency-----"
CLEAN_DB_PATH
outfile="$outfilepath/YCSB_4KB_tail_latency.out"
cmd="$ycsbc_path $input_path/4KB_busy_tail_latency >> $outfile"
echo $cmd
echo $cmd > "$outfile"
eval $cmd

echo "------------Sensitive Study------------"
echo "-------------Dataset Size-------------"
echo "----------------40GB------------------"
write_key_num="10000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
outfile="$outfilepath/dateset_size_40GB.out"
RUN_ONE_TEST

echo "----------------80GB------------------"
write_key_num="20000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
outfile="$outfilepath/dateset_size_80GB.out"
RUN_ONE_TEST

echo "----------------120GB------------------"
write_key_num="30000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
numa_nvm_next_node=5
outfile="$outfilepath/dateset_size_120GB.out"
RUN_ONE_TEST

echo "----------------160GB------------------"
write_key_num="40000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
numa_nvm_next_node=5
outfile="$outfilepath/dateset_size_160GB.out"
RUN_ONE_TEST

echo "----------------200GB------------------"
write_key_num="50000000"
size="4096"
test_type="fillrandom,stats,readrandom,stats,wait,stats"
numa_nvm_next_node=5
outfile="$outfilepath/dateset_size_200GB.out"
RUN_ONE_TEST

echo "-------------MemTable Size-------------"
numa_nvm_next_node=-1
echo "----------------64MB------------------"
write_key_num="20000000"
size="4096"
test_type="fillrandom,readrandom"
write_buffer_size=67108864
outfile="$outfilepath/memtable_size_64MB.out"
RUN_ONE_TEST

echo "----------------128MB------------------"
write_key_num="20000000"
size="4096"
test_type="fillrandom,readrandom"
write_buffer_size=134217728
outfile="$outfilepath/memtable_size_128MB.out"
RUN_ONE_TEST

echo "----------------256MB------------------"
write_key_num="20000000"
size="4096"
test_type="fillrandom,readrandom"
write_buffer_size=268435456
outfile="$outfilepath/memtable_size_256MB.out"
RUN_ONE_TEST

echo "----------------512MB------------------"
write_key_num="20000000"
size="4096"
test_type="fillrandom,readrandom"
write_buffer_size=536870912
outfile="$outfilepath/memtable_size_512MB.out"
RUN_ONE_TEST