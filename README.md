# Running, and Performance Test

MioDB can be performed db\_bench and YCSB as micro-benchmark and macro-benchmark to evaluate the performance. Since MioDB has no recovery functionality implemented yet, thus benchmark need re-run fill database for every benchmark.

## Micro-benchmark

For running db\_bench, we can refer to the script \textit{miodb\_test.sh} under test\_sh/. The options in the script:

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

Before running, we need to modify this script. First, we should modify the \textit{db\_path} to the path of NVM (the Optane DC PMMs can be mounted using command like \textit{mount /dev/pmem0/your/path}). Then, specify the path of the output file directory and output file by \textit{outfilepath} and \textit{outfile}. Third, we set the path of db\_bench binary path using \textit{bench\_path}. Finally, we modify the numa\_dram\_node and numa\_nvm\_node to the DRAM and NVM node, respectively. (using ''numactl -H'' in terminal to distinguish DRAM and NVM node). We can use the command below to run the YCSB test:

```
    [MioDB] sh miodb_test.sh
```
    
After the db\_bench runs, the throughput and latency will output in the \textit{outfilepath}. MioDB uses db\_bench to evaluate performance with value change in size and compare the throughput and latency with NoveLSM, MatrixKV, and etc.

For evaluating the performance of db\_bench (Figure 6 and Table 1 in the paper), we can modify the size of value by \textit{size} in the script, for example \textit{size=''4096''}. We can freely combine four kinds of test type (''fillrandom'', ''fillseq'', ''readrandom'', ''readseq'') for \textit{test\_type}, for example \textit{test\_type=''fillrandom,stats,readrandom,stats''}. Please make sure the write type is before the read type. The ''stats'' can print some DB status data. The cumulative stalls time, flushing time and write amplification will print in the terminal finally. We can use the function \textit{RUN\_ONE\_TEST} to run db\_bench once. The function \textit{RUN\_VALUE\_TEST} can run multi db\_bench with different value size in sequence. Before using RUN\_VALUE\_TEST, please set the \textit{value\_array} like \textit{value\_array=(1024 4096 16384 65536)}



## Macro-benchmark

For running YCSB, we can refer to the files under \textit{input/}. The options in the file:

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

We can set the workload options in the workload file. The options in the file:

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

Before running, we need to modify the files under  \textit{input/}. First, We set the \textit{-dbpath} to the path of NVM. Then, we set \textit{-P} to specify the workload file and choose load mode or run mode. We can use the command below to run the YCSB test:

```
    [MioDB/YCSB] ./ycsbc input/select_a_file
```
    
After the YCSB runs, it will output the throughput. MioDB uses YCSB to evaluate the tail latency and compare with NoveLSM, MatrixKV.

For evaluating the performance of YCSB (Figure 7 in the paper), we can modify the \textit{-P} to the workload load and A-F under the workloads/ directory (Note the distinction between 1KB and 4KB value size). For evaluating the tail latency of YCSB (Figure 8 and Table 2 in the paper), please use the ``tail.spec'' as the workload in run mode.

## Sensitivity Studies

For the sensitivity studies of levels (Figure 9 in the paper), we need recompile MioDB. The parameter which controls the level numbers of MioDB is in the file \textit{db/dbformat.h}. The name is \textit{kNumLevels} (line 26). We can modify it and then rebuild the hole program. The method of evaluating random write and random read performance is same as Micro-benchmark.

For the sensitivity studies of the size of dataset (Figure 10 and Figure 11 in the paper), we also use the \textit{miodb\_test.sh}. We can modify the \textit{write\_key\_num} to control the size of dataset. The total size can calculate by \textit{write\_key\_num} * \textit{size}. Because the size of key is very small compared to the value, we can just ignore the size of key.

## SSD-Supported MioDB Extension
We have implemented a simple demo of SSD-Supported MioDB. We can download the code from the github repository of MioDB on ''ssd\_extension'' branch. The compilation and evaluation method is same as the in-memory MioDB. The only difference between them is the \textit{db\_path} in db\_bench and the \textit{-dbpath} in YCSB. We need set these two parameters to a path of SSD.