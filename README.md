# MioDB: Devouring Data Byte-addressable LSM-based KV Stores for Hybrid Memory
&#160; &#160; &#160; &#160; MioDB is a byte-addressable LSM-based KV stores for hybrid memory. MioDB leverages the datable which is based on skiplist to replace the traditional persistent storage structure SSTable. Meanwhile, MioDB uses one-piece flush to convert data from DRAM to NVM quickly, data-reusable compaction to merge datable rapidly in low level, and deduplicated compaction in last level to finally delete the redundant data. MioDB also ultilizes an elastic buffer and a full-level parallel to merge concurrently to make data sink quickly. For optimizing read performance, MioDB can increase the number of levels and use bloom filters.

## Dependencies, Compilation and Run
### 1. External Dependencies
Before running MioDB, it's essential that you have already installed dependencies listing below.
* cmake (>=3.9.0)
* Intel Optane DC Persistent Memory configuration: install ipmctl & ndctl first, then use commands below. See [Details](https://stevescargall.com/2019/07/09/how-to-extend-volatile-system-memory-ram-using-persistent-memory-on-linux/?tdsourcetag=s_pctim_aiomsg).
```
$ sudo ipmctl delete -dimm -pcd
$ sudo ipmctl create -goal PersistentMemoryType=AppDirect
$ sudo systemctl reboot

$ ndctl create-namespace --mode=dax
$ daxctl migrate-device-model
$ udevadm trigger
$ daxctl reconfigure-device --mode=system-ram --no-online daxX.Y
```

### 2. Compilation
* Build MioDB via cmake
```
$ mkdir -p build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

### 3. Run
To test with db_bench, please refer to the test script
``test_sh/miodb_test.sh`` and run this script with command
```
$ sudo ./test_sh/miodb_test.sh
```