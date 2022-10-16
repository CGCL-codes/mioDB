#ifndef STORAGE_LEVELDB_DB_GLOBAL_H_
#define STORAGE_LEVELDB_DB_GLOBAL_H_
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "numa.h"
#include "leveldb/options.h"
using namespace leveldb;
extern int nvm_node;
extern int nvm_next_node;
extern size_t nvm_free_space;
extern bool nvm_node_has_changed;
extern size_t nvm_actual_use;
void NvmNodeSizeInit(const Options& options_);
void NvmNodeSizeRecord(size_t s);
void NvmFreeRecord(size_t s);
void NvmUsagePrint();
void OpenRecordFile();
void CloseRecordFile();
void RecordNvmSeries(); 
#endif
