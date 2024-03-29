# MioDB: Revisiting Log-structured Merging for KV Stores in Hybrid Memory Systems
&#160; &#160; &#160; &#160; MioDB is a byte-addressable LSM-based KV stores for hybrid memory. MioDB leverages the PMTable which is based on skiplist to replace the traditional persistent storage structure SSTable. Meanwhile, MioDB uses one-piece flush to convert data from DRAM to NVM quickly, zero-copy compaction to merge PMTable rapidly in low level, and lazy-copy compaction in last level to finally delete the redundant data. MioDB also ultilizes an elastic buffer and a full-level parallel to merge concurrently to make data sink quickly. For optimizing read performance, MioDB can increase the number of levels and use bloom filters.

&#160; &#160; &#160; &#160; MioDB is implemented on the base of Google [LevelDB](https://github.com/google/leveldb).

## Dependencies, Compilation and Running
### 1. External Dependencies
#### Hardware Dependencies
Our experiments use a server with hardware dependencies shown below.

```
    [CPU]: 2 X 20-core 2.10GHz Intel Xeon Gold 6230 with 28 MB LLC
    [DRAM]: 64 GB 2666 MHz DDR4 DRAM
    [NVM]: 8 X 128 GB Intel Optane DC PMM
```

#### Software Dependencies
Before running MioDB codes, it's essential that you have already install software dependencies shown below.

```
    g++ (GCC) 4.8.5
    numactl-devel
    ndctl
    cmake ($>=$3.9.0)
    snappy
```

#### Data Dependencies
We use two specific benchmarks, db_bench and YCSB, to evaluate MioDB. The source code of benchmarks are included in our code repository (in folds *MioDB/benchmarks* and *MioDB/ycsbc*).

### 2. Compilation
For running MioDB, the entire project needs to be compiled. We can build MioDB via cmake. The command to compile the MioDB:
    
```
    [MioDB] mkdir -p build && cd build
    [MioDB] cmake -DCMAKE_BUILD_TYPE=Release .. && make
```
    
Then the static library of MioDB and the executable db_bench program will be generated in the build/ folder.

We can build YCSB via Makefile. Before compiling, we need to set some environment variables. We can modify and use *ycsbc/ycsb_build_env.sh* to set these.

```
    // modify the script
    CPLUS_INCLUDE_PATH=the absolute path of MioDB/include
    LIBRARY_PATH=the absolute path of MioDB/build
    // execute the command
    [user@node ~] source ycsbc/ycsb_build_env.sh
```

Of course, we can also directly execute the commands below in the terminal to set the environment variables before every complation. Assume the absolute path of MioDB is */home/user/mioDB*.

```
[user@node ~] export LIBRARY_PATH=/home/user/mioDB/build
[user@node ~] export CPLUS_INCLUDE_PATH=/home/user/mioDB/include
```

Use `env` command to check the environment variables.

```
[user@node ~] env
```
    
Meanwhile, we need modify some source code in *ycsbc/db/leveldb_db.cc*. We should set the *options->nvm_node* (line 34) to the numa node of NVM (using ''numactl -H'' in terminal to distinguish DRAM and NVM node). The command to compile the YCSB:

```
    [MioDB/YCSB] make
```

### 3. Running, and Performance Testing

MioDB can be performed db_bench and YCSB as micro-benchmark and macro-benchmark to evaluate the performance. Since MioDB does not support checkpointing, we have to re-run each benchmark to fill the database for each test.

#### Micro-benchmark

For running db_bench, we can refer to the script *miodb_test.sh* under *test_sh/*. The options are listed in the script:

```
    [db_path]: save LOG, CURRENT, MANIFEST, and etc.
    [outfilepath]: output file dir.
    [outfile]: output file.
    [bench_path]: db_bench binary path.
    [test_size]: total size of workload (Byte).
    [size]: value size (Byte).
    [value_array]: multi values in once execution.
    [write_key_num]: the number of writing KV (test_size / size).
    [read_key_num]: the number of reading KV.
    [test_type]: test type in db_bench.
    [bench_keys_per_datatable]: theoretical KV number in a PMTable of penultimate layer.
    [numa_dram_node]: dram node in numa.
    [numa_nvm_node]: nvm node in numa.
    [numa_nvm_next_node]: nvm next node in numa, we can use -1 to disable it.
    [write_buffer_size]: the size of MemTable in DRAM (Byte).
```

Before running, we need to modify this script. First, we should modify the *db_path* to the path of NVM (the Optane DC PMMs can be mounted using command like *mount /dev/pmem0/your/path*). Then, we should specify the path of the output directory and output file by *outfilepath* and *outfile*. Third, we set the path of db_bench using *bench_path*. Finally, we modify the numa_dram_node and numa_nvm_node to the DRAM and NVM node, respectively (using ''numactl -H'' in terminal to distinguish DRAM and NVM node). We can use the command below to run the db_bench test:

```
    [MioDB] sudo sh miodb_test.sh
    // Another way to run this script 
    [MioDB] sudo chmod 775 miodb_test.sh
    [MioDB] sudo ./miodb_test.sh
```

After the db_bench finishes, the throughput and latency results are recorded in the *outfilepath*. We use db_bench to evaluate the performance with different value sizes and compare the throughput and latency of MioDB with that of NoveLSM and MatrixKV.

For evaluating the performance of db_bench (Figure 6 and Table 1 in the paper), we can modify the size of value by *size* in the script, for example, *size=''4096''*. We can freely combine four kinds of test types (''fillrandom'', ''fillseq'', ''readrandom'', ''readseq'') to configure the *test_type*, for example:

```
     test_type=''fillrandom,stats,readrandom,stats''
```
Please make sure the write type is before the read type. The ''stats'' can print some DB status data. The cumulative stalls time, flushing time and write amplification ratio will be printed in the terminal finally. 

We can use the function *RUN_ONE_TEST* to run all db_bench tests. The function *RUN_VALUE_TEST* can run multi db_bench with different value sizes in sequence. Before using *RUN_VALUE_TEST*, please set the *value_array* at first, for example,  *value_array=(1024 4096 16384 65536)*.

The output format of db_bench is below. The `fillrandom` means random write latency and throughput. The `readrandom` means random read latency and throughput. The `stall time` means the time for which the write operations are blocked (microseconds). The `dump time` means the flushing time (microseconds). The `wa` means the total write size into NVM (Bytes). We can calculate the write amplification ratio use `wa / (num * value_size) + 1`.

```
/home/jbyao/mioDB/build/db_bench --num=20000000 --value_size=4096 --benchmarks=fillrandom,readrandom --reads=1000000 --compression_ratio=1 --db=/mnt/persist-memory/nvm --dram_node=0 --nvm_node=4 --nvm_next_node=-1 --write_buffer_size=67108864 >> /home/jbyao/auto_result/db_bench_random_4KB.out
Keys:       16 bytes each
Values:     4096 bytes each (4096 bytes after compression)
Entries:    20000000
RawSize:    78430.2 MB (estimated)
FileSize:   78430.2 MB (estimated)
------------------------------------------------
stall time: 0
dump time:  0
wa: 0
fillrandom   :      10.300 micros/op;  380.7 MB/s
readrandom   :      34.531 micros/op;  113.6 MB/s (1000000 of 20000000 found)
stall time: 27854350
dump time:  14296363
wa: 120001366345
```

#### Macro-benchmark

For running YCSB, we can refer to the files under *input/*. The options are listed in the file:

```
    [-db]: MioDB uses "leveldb".
    [-dbpath]: save LOG, CURRENT, MANIFEST, and etc.
    [-threads]: number of test threads.
    [-P]: the YCSB workload.
    [-load/-run]: load mode or run mode.
    [-dboption]: set MioDB parameters. 
    [-dbstatistics]: whether to print benchmark data.
    [-dbwaitforbalance]: whether to wait for database balance.
    [-timeseries]: whether to print time series.
```

We can set the workload options in the workload file. The options are listed in the file:

```
    [fieldcount]: the number of values.
    [fieldlength]: the size of values (Byte).
    [recordcount]: the number of records.
    [operationcount]: the number of operations.
    [readallfields]: whether read all fields of a value.
    [readproportion]: the proportion of read operations. 
    [updateproportion]: the proportion of update operations. 
    [scanproportion]: the proportion of scan operations.
    [insertproportion]: the proportion of insert operations.
    [requestdistribution]: the destribution of the workload.
```

Before running, we need to modify the files under *input/*. First, We set the *-dbpath* to the path of NVM. Then, we set *-P* to specify the workload file and choose the load mode or the run mode. We can use the command below to run the YCSB test:

```
    [MioDB/YCSB] ./ycsbc input/select_a_file
```
    
After the YCSB finishes, it will output the throughput. MioDB uses YCSB to evaluate the tail latency and compare with NoveLSM, MatrixKV.

For evaluating the performance of YCSB (Figure 7 in the paper), we can modify the *-P* to the workload load and A-F under the workloads/ directory (Note the distinction between 1KB and 4KB value size). For evaluating the tail latency of YCSB (Figure 8 and Table 2 in the paper), please use the ''tail.spec'' as the workload in the run mode.

The output file of YCSB is long. We just show some key results in the file. The results below correspond to the IOPS of Load, A, B, C, D, E, and F from top to bottom.

```
********** load result **********
loading records:20000000  use time:253.431 s  IOPS:78917.06 iops
********** run result **********
all opeartion records:100000  use time:1.466 s  IOPS:68233.40 iops

read ops  :  49894  use time:  0.754 s  IOPS:66186.28 iops
update ops:  50106  use time:  0.705 s  IOPS:71042.71 iops
********** run result **********
all opeartion records:100000  use time:1.110 s  IOPS:90116.48 iops

read ops  :  95030  use time:  1.036 s  IOPS:91698.86 iops
update ops:   4970  use time:  0.070 s  IOPS:71098.52 iops
********** run result **********
all opeartion records:100000  use time:1.022 s  IOPS:97834.15 iops

read ops  : 100000  use time:  1.019 s  IOPS:98164.91 iops
********** run result **********
all opeartion records:100000  use time:2.154 s  IOPS:46420.97 iops

insert ops:   5026  use time:  0.073 s  IOPS:68686.54 iops
read ops  :  94974  use time:  2.077 s  IOPS:45715.90 iops
********** run result **********
all opeartion records:100000  use time:42.476 s  IOPS:2354.26 iops

insert ops:   5014  use time:  0.151 s  IOPS:33305.88 iops
scan ops  :  94986  use time: 42.320 s  IOPS:2244.47 iops
********** run result **********
all opeartion records:100000  use time:1.780 s  IOPS:56169.24 iops

read ops  :  49927  use time:  0.587 s  IOPS:85027.14 iops
rmw ops   :  50073  use time:  1.188 s  IOPS:42137.46 iops
```

#### Sensitivity Studies

For the sensitivity studies of levels (Figure 9 in the paper), we need recompile MioDB. The parameter which controls the level numbers of MioDB is in the file *db/dbformat.h*. The name is *kNumLevels* (line 26). We can modify it and then rebuild the whole program. The method of evaluating the random write and random read performance is same as the above Micro-benchmark.

For the sensitivity studies of the size of dataset (Figure 10 and Figure 11 in the paper), we also use the *miodb\_test.sh*. We can modify the *write_key_num* to configure the size of dataset. The total size can be calculated by *write_key_num* * *size*. Because the size of key is very small compared to the value, we can ignore the size of key.

#### SSD-Supported MioDB Extension
We have implemented SSD-Supported MioDB. We can download the code from the github repository of MioDB on ''ssd_extension'' branch. The compilation and evaluation method is same as the in-memory MioDB. The only difference between them is the *db_path* in db_bench and the *-dbpath* in YCSB. We need set these two parameters to a data path of SSD.

#### Auto Scripts

We also provide two auto scripts to reproduce the results in our paper. *test_sh/auto.sh* can automatically execute the db_bench and YCSB in the order of the paper for in-memory MioDB. *test_sh/parser.sh* can parse the result of *test_sh/auto.sh*. Before use these two scripts, we need modify some neccessary parameters at the beginning of the scripts. The most parameters are the same meaning as them in *test_sh/miodb_test.sh*. We also write some notes in the scripts, please check them before.