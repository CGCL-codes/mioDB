# MioDB
MioDB: Devouring Data Byte-addressable LSM-based KV Stores for Hybrid Memory

## Compilation and Run
### Tools
MioDB access NVM via NUMA

### Compilation
1. modify ``db/skiplist.h`` & ``util/arena.h``
```
> #define NUMA_NODE your_nvm_node_num
```
2. build MioDB via cmake
```
> mkdir -p build && cd build
> cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

### Run
To test with db_bench, please refer to the test script
``test_sh/miodb_test.sh`` and run this script with command
```
> ./test_sh/miodb_test.sh
```