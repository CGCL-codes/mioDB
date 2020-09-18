#ifndef STORAGE_LEVELDB_DB_GLOBAL_H_
#define STORAGE_LEVELDB_DB_GLOBAL_H_
#include <stdlib.h>
#include <iostream>
#include "numa.h"
#include "leveldb/options.h"
using namespace leveldb;
extern int nvm_node;
extern int nvm_next_node;
extern size_t nvm_free_space;
extern bool nvm_node_has_changed;
void NvmNodeSizeInit(const Options& options_);
void NvmNodeSizeRecord(size_t s);
#endif
